#ifndef klox_vm_h
#define klox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 1024

typedef struct {
     chunk* chunk;                 //the chunk fed to the VM to be interpreted
     uint8_t* ip;                  //instruction pointer
     value stack[STACK_MAX];
     value* stack_top;
     hash_table strings;
     obj* objects;
} VM;

typedef enum {
     RESULT_OK,
     RESULT_COMPILE_ERROR,
     RESULT_RUNTIME_ERROR
} result;

extern VM vm;

void init_vm();
void free_vm();
result interpret(const char* source);
void push(value value);
value pop();

#endif
