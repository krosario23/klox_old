#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void init_chunk(chunk* chunk) {
     chunk->count = 0;
     chunk->capacity = 0;
     chunk->code = NULL;
     chunk->lines = NULL;
     init_val_array(&chunk->constants);
}

void write_chunk(chunk* chunk, uint8_t byte, int line) {
     if (chunk->capacity < chunk->count + 1) {
          int old_capacity = chunk->capacity;
          chunk->capacity = GROW_CAPACITY(old_capacity);
          chunk->code = GROW_ARRAY(chunk->code, uint8_t,
                                   old_capacity, chunk->capacity);
          chunk->lines = GROW_ARRAY(chunk->lines, int,
                                   old_capacity, chunk->capacity);
     }

     chunk->code[chunk->count] = byte;

     chunk->lines[chunk->count] = line;
     chunk->count++;
}

int add_constant(chunk* chunk, value val) {
     /*   this function adds the given value to the constant pool
          and returns the index of the added constant for later use   */

     write_val_array(&chunk->constants, val);
     return chunk->constants.count - 1;
}

void free_chunk(chunk* chunk) {
     FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
     FREE_ARRAY(int, chunk->lines, chunk->capacity);
     free_val_array(&chunk->constants);
     init_chunk(chunk);
}
