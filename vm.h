#ifndef klox_vm_h
#define klox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 1024

typedef struct {
     chunk* chunk;
     uint8_t* ip;
     value stack[STACK_MAX];
     value* stack_top;
} VM;

typedef enum {
     RESULT_OK,
     RESULT_COMPILE_ERROR,
     RESULT_RUNTIME_ERROR
} result;

void init_vm();
void free_vm();
result interpret(const char* source);
void push(value value);
value pop();

#endif
