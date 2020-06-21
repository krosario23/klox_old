#include <stdlib.h>

#include "common.h"
#include "memory.h"
#include "vm.h"

void* reallocate(void* previous, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(previous);
        return NULL;
    }
    
    return realloc(previous, new_size);
}

static void free_object(obj* object) {
    switch(object->type) {
        case OBJ_STRING: {
            obj_string* string = (obj_string*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(obj_string, object);
            break;
        }
    }
}

void free_objects() {
    obj* object = vm.objects;
    while (object != NULL) {
        obj* next = object->next;
        free_object(object);
        object = next;
    }
}
