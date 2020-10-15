#ifndef klox_object_h
#define klox_object_h

#include "common.h"
#include "value.h"

//helper macro to get the type of a ovject value
#define OBJ_TYPE(val)         (AS_OBJ(val)->type)

//verifies that the given value is actually a string
#define IS_STRING(val)        is_obj_type(val, OBJ_STRING)

//unwrap the given klox object into a klox string object
#define AS_STRING(val)        ((obj_string*)AS_OBJ(val))

//unwrap the given klox object into a native C string
#define AS_CSTRING(val)       (((obj_string*)AS_OBJ(val))->chars)

typedef enum {
     OBJ_STRING,
} obj_type;

//a sort of abstract base struct for our different type of objects
struct s_obj {
     obj_type type;
     struct s_obj* next;
};

struct s_obj_string {
     obj object;
     int length;
     char* chars;
     uint32_t hash;
};

obj_string* take_string(char* chars, int length);
obj_string* copy_string(const char* chars, int length);
void print_object(value val);

//verifies that the given value is actually of the given type
static inline bool is_obj_type(value val, obj_type type) {
     return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

#endif
