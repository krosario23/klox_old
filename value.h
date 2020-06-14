#ifndef klox_value_h
#define klox_value_h

#include "common.h"

typedef struct s_obj obj;
typedef struct s_obj_string obj_string;

typedef enum {
     VAL_BOOL,
     VAL_NULL,
     VAL_NUMBER,
     VAL_OBJ,
} val_type;

//a tagged union that lets us represent different values
typedef struct {
     val_type type;           //type tag
     union {                  //union field that contains underlying values
          bool boolean;
          double number;
          obj* object;
     } as;
} value;

//these macros verify that we are using are the correct type
#define IS_BOOL(val)    ((val).type == VAL_BOOL)
#define IS_NULL(val)    ((val).type == VAL_NULL)
#define IS_NUMBER(val)  ((val).type == VAL_NUMBER)
#define IS_OBJ(val)     ((val).type == VAL_OBJ)

//these macros unwrap klox values back to C values
#define AS_OBJ(val)     ((val).as.object)
#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUMBER(val)  ((val).as.number)

//these macros promots native C values to klox values
#define BOOL_VAL(val)   ((value){ VAL_BOOL, { .boolean = val } })
#define NULL_VAL        ((value){ VAL_NULL, { .number = 0 } })
#define NUMBER_VAL(val) ((value){ VAL_NUMBER, { .number = val } })
#define OBJ_VAL(val)    ((value){ VAL_OBJ, { .object = (obj*)val } })

//val_array represents the constant pool associated with each chunk
typedef struct {
     int count;
     int capacity;
     value* values;
} val_array;

bool values_equal(value a, value b);
void init_val_array(val_array* array);
void write_val_array(val_array* array, value val);
void free_val_array(val_array* array);
void print_value(value val);

#endif
