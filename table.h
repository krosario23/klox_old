#ifndef klox_table_h
#define klox_table_h

#include "common.h"
#include "value.h"

typedef struct {
     obj_string* key;
     value val;
} entry;

typedef struct {
     int count;
     int capacity;
     entry* entries;
} hash_table;

void init_table(hash_table* table);
void free_table(hash_table* table);
bool table_get(hash_table* table, obj_string* key, value* val);
bool table_set(hash_table* table, obj_string* key, value val);
bool table_delete(hash_table* table, obj_string* key);
void table_add_all(hash_table* from, hash_table* to);
obj_string* table_find_string(hash_table* table, const char* chars, int length,
                              uint32_t hash);

#endif
