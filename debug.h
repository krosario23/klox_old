#ifndef klox_debug_h
#define klox_debug_h

#include "chunk.h"

void disassemble_chunk(chunk* chunk, const char* name);
int disassemble_instruction(chunk* chunk, int offset);

#endif
