#include <stdio.h>

#include "memory.h"
#include "value.h"

void init_val_array(val_array* array) {
     array->count = 0;
     array->capacity = 0;
     array->values = NULL;
}

void write_val_array (val_array* array, value val) {
     if (array->capacity < array->count + 1) {
          int old_capacity = array->capacity;
          array->capacity = GROW_CAPACITY(old_capacity);
          array->values = GROW_ARRAY(array->values, value,
                                     old_capacity, array->capacity);
     }

     array->values[array->count] = val;
     array->count++;
}


void free_val_array(val_array* array) {
     FREE_ARRAY(value, array->values, array->capacity);
     init_val_array(array);
}

void print_value(value val) {
     printf("%g", val);
}
