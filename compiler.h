#ifndef klox_compiler_h
#define klox_compiler_h

#include "object.h"
#include "vm.h"

bool compile(const char* source, chunk* chunk);

#endif
