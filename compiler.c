#include <stdio.h>
#include <stdlib.h>

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

typedef void (*parse_fn)();

typedef struct {
     parse_fn prefix;
     parse_fn infix;
     precedence prec;
} parse_rule;

parser parse;

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

static void end_compiler() {
     emit_return();
#ifdef DEBUG_PRINT_CODE
     if (!parse.had_error) {
          disassemble_chunk(current_chunk(), "code");
     }
#endif
}

/*-------------------------- more parsing functions --------------------------*/

static void expression();
static parse_rule* get_rule(token_type type);
static void parse_prec(precedence prec);

static void binary() {
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

static void literal() {
     switch (parse.previous.type) {
          case TOKEN_FALSE: emit_byte(OP_FALSE); break;
          case TOKEN_NULL:  emit_byte(OP_NULL); break;
          case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
          default:
               return;
     }
}

static void grouping() {
     expression();
     consume(TOKEN_RIGHT_PAREN, "expected ')' after expression");
}

static void number() {
     double val = strtod(parse.previous.start, NULL);
     emit_constant(NUMBER_VAL(val));
}

static void string() {
     emit_constant(OBJ_VAL(copy_string(parse.previous.start + 1,
                                       parse.previous.length - 2)));
}

static void unary() {
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
     { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
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

     prefix_rule();

     while (prec <= get_rule(parse.current.type)->prec) {
          advance();
          parse_fn infix_rule = get_rule(parse.previous.type)->infix;
          infix_rule();
     }
}

static parse_rule* get_rule(token_type type) {
     return &rules[type];
}

static void expression() {
     parse_prec(PREC_ASSIGNMENT);
}

bool compile(const char* source, chunk* chunk) {
     init_scanner(source);
     compiling = chunk;
     parse.had_error = false;
     parse.panic = false;
     advance();
     expression();
     consume(TOKEN_EOF, "expected end of expression");
     end_compiler();
     return !parse.had_error;
}
