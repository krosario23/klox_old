#ifndef klox_value_h
#define klox_value_h

#include "common.h"

typedef double value;

//val_array represents the constant pool associated with each chunk
typedef struct {
     int count;
     int capacity;
     value* values;
} val_array;

void init_val_array(val_array* array);
void write_val_array(val_array* array, value val);
void free_val_array(val_array* array);
void print_value(value val);


#endif
