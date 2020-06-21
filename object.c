#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h" 
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
    (type*)allocate_object(sizeof(type), object_type)

/*   allocates memory on the heap for a given object and adds it to the linked
 list of objects for keeping track of refrences */
static obj* allocate_object(size_t size, obj_type type) {
    obj* object = (obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

//allocates an obj_string on the heap and returns a pointer to that object
static obj_string* allocate_string(char* chars, int length, uint32_t hash) {
    obj_string* string = ALLOCATE_OBJ(obj_string, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    
    table_set(&vm.strings, string, NULL_VAL);
    
    return string;
}

static uint32_t hash_string(const char* key, int length) {
    uint32_t hash = 2166136261u;
    
    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    
    return hash;
}

obj_string* take_string(char* chars, int length) {
    uint32_t hash = hash_string(chars, length);
    
    obj_string* interned = table_find_string(&vm.strings, chars, length, hash);
    
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    
    return allocate_string(chars, length, hash);
}

/*   allocates a new array on the heap, then copies over the given string into
 the array, adding a null terminator at the end */
obj_string* copy_string(const char* chars, int length) {
    uint32_t hash = hash_string(chars, length);
    obj_string* interned = table_find_string(&vm.strings, chars, length, hash);
    
    if (interned != NULL) return interned;
    
    char* heap = ALLOCATE(char, length + 1);
    memcpy(heap, chars, length);
    heap[length] = '\0';
    
    return allocate_string(heap, length, hash);
}

void print_object(value val) {
    switch (OBJ_TYPE(val)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(val));
            break;
    }
}
