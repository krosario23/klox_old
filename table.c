#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_table(hash_table* table) {
    FREE_ARRAY(entry, table->entries, table->capacity);
    init_table(table);
}

static entry* find_entry(entry* entries, int capacity, obj_string* key) {
    uint32_t index = key->hash % capacity;
    entry* tombstone = NULL;
    for (;;) {
        entry* ent = &entries[index];
        
        if (ent->key == NULL) {
            if (IS_NULL(ent->val)) {
                //empty entry
                return tombstone != NULL ? tombstone : ent;
            } else {
                //found a tombstone
                if (tombstone == NULL) tombstone = ent;
            }
        } else if (ent->key == key) {
            //found the key
            return ent;
        }
        
        index = (index + 1) % capacity;
    }
}

bool table_get(hash_table* table, obj_string* key, value* val) {
    if (table->count == 0) return false;
    
    entry* ent = find_entry(table->entries, table->capacity, key);
    if (ent->key == NULL) return false;
    
    *val = ent->val;
    return true;
}

static void adjust_capacity(hash_table* table, int capacity) {
    entry* entries = ALLOCATE(entry, capacity);
    
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].val = NULL_VAL;
    }
    
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        entry* ent = &table->entries[i];
        if (ent->key == NULL) continue;
        
        entry* dest = find_entry(entries, capacity, ent->key);
        dest->key = ent->key;
        dest->val = ent->val;
        table->count++;
    }
    
    
    FREE_ARRAY(entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool table_set(hash_table* table, obj_string* key, value val) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }
    
    entry* ent = find_entry(table->entries, table->capacity, key);
    
    bool is_new = (ent->key == NULL);
    if (is_new && IS_NULL(ent->val)) table->count++;
    
    ent->key = key;
    ent->val = val;
    return is_new;
}

bool table_delete(hash_table* table, obj_string* key) {
    if (table->count == 0) return false;
    
    entry* ent = find_entry(table->entries, table->capacity, key);
    if (ent->key == NULL) return false;
    
    ent->key = NULL;
    ent->val = BOOL_VAL(true);
    
    return true;
}

void table_add_all(hash_table* from, hash_table* to) {
    for (int i = 0; i < from->capacity; i++) {
        entry* ent = &from->entries[i];
        if (ent->key != NULL) {
            table_set(to, ent->key, ent->val);
        }
    }
}

obj_string* table_find_string(hash_table* table, const char* chars, int length,
                              uint32_t hash) {
    if (table->count == 0) return NULL;
    
    uint32_t index = hash % table->capacity;
    
    for (;;) {
        entry* ent = &table->entries[index];
        
        if (ent->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NULL(ent->val)) return NULL;
        } else if (ent->key->length == length &&
                   ent->key->hash == hash &&
                   memcmp(ent->key->chars, chars, length) == 0) {
            // We found it.
            return ent->key;
        }
        
        index = (index + 1) % table->capacity;
    }
}
