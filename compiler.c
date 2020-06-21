#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    token current;
    token previous;
    bool had_error;
    bool panic;
} parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} precedence;

typedef void (*parse_fn)(bool can_assign);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence prec;
} parse_rule;

typedef struct {
    token name;
    int depth;
} local;

typedef struct compiler {
    local locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
} compiler;

parser parse;
compiler* current = NULL;

//the chunk that we are currently compiling to
chunk* compiling;

static chunk* current_chunk() {
    return compiling;
}

/*------------------------ error reporting functions -------------------------*/

static void error_at(token* token, const char* message) {
    if (parse.panic) return;
    parse.panic = true;
    fprintf(stderr, "[line %d] error", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        //nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    parse.had_error = true;
}

static void error(const char* message) {
    error_at(&parse.previous, message);
}

static void error_at_current(const char* message) {
    error_at(&parse.current, message);
}

/*---------------------------- parsing functions -----------------------------*/

/*   this function stores the previous token in the 'previous' field and
 loops through the token stream */
static void advance() {
    parse.previous = parse.current;
    
    for (;;) {
        parse.current = scan_token();
        if (parse.current.type != TOKEN_ERROR) break;
        error_at_current(parse.current.start);
    }
}

/*   advances to next token while checking for a given type, if the type is
 wrong we report a syntax error */
static void consume(token_type type, const char* message) {
    if (parse.current.type == type) {
        advance();
        return;
    }
    
    error_at_current(message);
}

static bool check(token_type type) {
    return parse.current.type == type;
}

static bool match(token_type type) {
    if (!check(type)) return false;
    advance();
    return true;
}

/*---------------------------- codegen functions -----------------------------*/

//the following functions emit bytecode into the current chunk
static void emit_byte(uint8_t byte){
    write_chunk(current_chunk(), byte, parse.previous.line);
}

static void emit_return() {
    emit_byte(OP_RETURN);
}

static uint8_t make_constant(value val) {
    int constant = add_constant(current_chunk(), val);
    if (constant > UINT8_MAX) {
        error("too many constants in one chunk");
        return 0;
    }
    
    return (uint8_t)constant;
}

//this one emits 2 instructions for opcodes with operands
static void emit_bytes(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_constant(value val) {
    emit_bytes(OP_CONSTANT, make_constant(val));
}

static void init_compiler(compiler* comp) {
    comp->local_count = 0;
    comp->scope_depth = 0;
    current = comp;
}

static void end_compiler() {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parse.had_error) {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void begin_scope() {
    current->scope_depth++;
}

static void end_scope() {
    current->scope_depth--;
    
    while (current->local_count > 0 &&
           current->locals[current->local_count - 1].depth > current->scope_depth) {
        emit_byte(OP_POP);
        current->local_count--;
    }
}

/*-------------------------- more parsing functions --------------------------*/

static void expression(void);
static void statement(void);
static void declaration(void);
static parse_rule* get_rule(token_type type);
static void parse_prec(precedence prec);

static uint8_t identifier_constant(token* name) {
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static bool identifiers_equal(token* a, token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(compiler* comp, token* name) {
    for (int i = comp->local_count - 1; i >= 0; i--) {
        local* loc = &comp->locals[i];
        if (identifiers_equal(name, &loc->name)) {
            if (loc->depth == -1) {
                error("cannot read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static void add_local(token name) {
    if (current->local_count == UINT8_COUNT) {
        error("too many local variables in this function");
        return;
    }
    local* loc = &current->locals[current->local_count++];
    loc->name = name;
    loc->depth = -1;
}

static void declare_variable() {
    if (current->scope_depth == 0) return;
    
    token* name = &parse.previous;
    
    for (int i = current->local_count - 1; i >= 0; i--) {
        local* loc = &current->locals[i];
        if (loc->depth != -1 && loc->depth < current->scope_depth) {
            break;
        }
        
        if (identifiers_equal(name, &loc->name)) {
            error("variable with this name already declared in this scope");
        }
    }
    add_local(*name);
}

static void binary(bool can_assign) {
    token_type operator = parse.previous.type;
    
    parse_rule* rule = get_rule(operator);
    parse_prec((precedence)(rule->prec + 1));
    
    switch (operator) {
        case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
        case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emit_byte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emit_byte(OP_ADD); break;
        case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
        default:
            return;
    }
}

static void literal(bool can_assign) {
    switch (parse.previous.type) {
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_NULL:  emit_byte(OP_NULL); break;
        case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
        default:
            return;
    }
}

static void grouping(bool can_assign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after expression");
}

static void number(bool can_assign) {
    double val = strtod(parse.previous.start, NULL);
    emit_constant(NUMBER_VAL(val));
}

static void string(bool can_assign) {
    emit_constant(OBJ_VAL(copy_string(parse.previous.start + 1,
                                      parse.previous.length - 2)));
}

static void named_variable(token name, bool can_assign) {
    uint8_t get_op, set_op;
    int arg = resolve_local(current, &name);
    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(set_op, (uint8_t)arg);
    } else {
        emit_bytes(get_op, (uint8_t)arg);
    }
}

static void variable(bool can_assign) {
    named_variable(parse.previous, can_assign);
}

static void unary(bool can_assign) {
    token_type operator = parse.previous.type;
    
    //compile the operand
    parse_prec(PREC_UNARY);
    
    //emit operator instruction
    switch (operator) {
        case TOKEN_BANG:  emit_byte(OP_NOT); break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        default:
            return;
    }
}

parse_rule rules[] = {
    { grouping, NULL,    PREC_NONE },       // TOKEN_LEFT_PAREN
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
    { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
    { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
    { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
    { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
    { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
    { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG_EQUAL
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
    { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
    { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
    { string,   NULL,    PREC_NONE },       // TOKEN_STRING
    { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
    { NULL,     NULL,    PREC_NONE },       // TOKEN_AND
    { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
    { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
    { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FUNC
    { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
    { literal,  NULL,    PREC_NONE },       // TOKEN_NULL
    { NULL,     NULL,    PREC_NONE },       // TOKEN_OR
    { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
    { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
    { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_LET
    { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static void parse_prec(precedence prec) {
    advance();
    
    //here we look up the previous token type in the parse table
    parse_fn prefix_rule = get_rule(parse.previous.type)->prefix;
    
    if (prefix_rule == NULL) {
        error("expected expression");
        return;
    }
    
    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);
    
    while (prec <= get_rule(parse.current.type)->prec) {
        advance();
        parse_fn infix_rule = get_rule(parse.previous.type)->infix;
        infix_rule(can_assign);
    }
    
    if (can_assign && match(TOKEN_EQUAL)) {
        error("invalid assignment target");
    }
}

static uint8_t parse_variable(const char* error_msg) {
    consume(TOKEN_IDENTIFIER, error_msg);
    declare_variable();
    if (current->scope_depth > 0) return 0;
    return identifier_constant(&parse.previous);
}

static void mark_initialized() {
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(uint8_t global) {
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

static parse_rule* get_rule(token_type type) {
    return &rules[type];
}

static void expression() {
    parse_prec(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "expected '}' after block");
}

static void var_declaration() {
    uint8_t global = parse_variable("expected variable name");
    
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NULL);
    }
    
    consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    define_variable(global);
}

static void expression_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_POP);
}

static void print_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_PRINT);
}

static void synchronize() {
    parse.panic = false;
    
    while (parse.current.type != TOKEN_EOF) {
        if (parse.previous.type == TOKEN_SEMICOLON) return;
        
        switch (parse.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUNC:
            case TOKEN_LET:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ;
        }
        
        advance();
    }
}

static void declaration() {
    if (match(TOKEN_LET)) {
        var_declaration();
    } else {
        statement();
    }
    
    if (parse.panic) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

bool compile(const char* source, chunk* chunk) {
    init_scanner(source);
    compiler comp;
    init_compiler(&comp);
    compiling = chunk;
    parse.had_error = false;
    parse.panic = false;
    advance();
    
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    
    consume(TOKEN_EOF, "expected end of expression");
    end_compiler();
    return !parse.had_error;
}
