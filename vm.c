#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

static void reset_stack() {
    vm.stack_top = vm.stack;
}

//function for reporting runtime errorss
static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    
    reset_stack();
}

void init_vm() {
    reset_stack();
    vm.objects = NULL;
    init_table(&vm.globals);
    init_table(&vm.strings);
}

void free_vm() {
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_objects();
}

void push(value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

value pop() {
    vm.stack_top--;
    return *vm.stack_top;
}

static value peek(int distance) {
    return vm.stack_top[-1 - distance];
}

//null and false should evaluate to false, everything else is true
static bool is_falsey(value val) {
    return IS_NULL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

static void concatenate() {
    obj_string* b = AS_STRING(pop());
    obj_string* a = AS_STRING(pop());
    
    int length = a->length + b->length;
    char* chars =  ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    
    obj_string* result = take_string(chars, length);
    push(OBJ_VAL(result));
}

static result run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(val_type, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtime_error("operands must be numbers"); \
            return RESULT_RUNTIME_ERROR; \
        } \
        \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(val_type(a op b)); \
    } while (false)
    
    
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("            ");
        
        for (value* slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        
        printf("\n");
        
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NULL:   push(NULL_VAL); break;
            case OP_TRUE:   push(BOOL_VAL(true)); break;
            case OP_FALSE:  push(BOOL_VAL(false)); break;
            case OP_POP:    pop(); break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_GET_GLOBAL: {
                obj_string* name = READ_STRING();
                value val;
                if (!table_get(&vm.globals, name, &val)) {
                    runtime_error("undefined variable '%s'", name->chars);
                    return RESULT_RUNTIME_ERROR;
                }
                push(val);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                obj_string* name = READ_STRING();
                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                obj_string* name = READ_STRING();
                if (table_set(&vm.globals, name, peek(0))) {
                    table_delete(&vm.globals, name);
                    runtime_error("undefined variable '%s'", name->chars);
                    return RESULT_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL:  {
                value b = pop();
                value a = pop();
                push(BOOL_VAL(values_equal(a, b)));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtime_error("operand must be a number");
                    return RESULT_RUNTIME_ERROR;
                }
                
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtime_error("operands to addition must be numbers or strings");
                    return RESULT_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: {
                push(BOOL_VAL(is_falsey(pop())));
                break;
            }
            case OP_PRINT: {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return RESULT_OK;
            }
        }
    }
    
#undef READ_BYTE
#undef READ_CONSTANT
#undef REAS_STRING
#undef BINARY_OP
}

result interpret(const char* source) {
    chunk chunk;
    init_chunk(&chunk);
    
    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return RESULT_COMPILE_ERROR;
    }
    
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    
    result result = run();
    
    free_chunk(&chunk);
    return result;
}
