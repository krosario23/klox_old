#ifndef klox_chunk_h
#define klox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
     OP_CONSTANT,
     OP_NULL,
     OP_TRUE,
     OP_FALSE,
     OP_POP,
     OP_GET_LOCAL,
     OP_SET_LOCAL,
     OP_GET_GLOBAL,
     OP_DEFINE_GLOBAL,
     OP_SET_GLOBAL,
     OP_EQUAL,
     OP_GREATER,
     OP_LESS,
     OP_ADD,
     OP_SUBTRACT,
     OP_MULTIPLY,
     OP_DIVIDE,
     OP_NOT,
     OP_NEGATE,
     OP_PRINT,
     OP_RETURN,
} opcode;

//chunk represents a block of bytecode
typedef struct {
     int count;
     int capacity;
     uint8_t* code;
     int* lines;
     val_array constants;
} chunk;


void init_chunk(chunk* chunk);
void write_chunk(chunk* chunk, uint8_t byte, int line);
int add_constant(chunk* chunk, value val);
void free_chunk(chunk* chunk);

#endif
