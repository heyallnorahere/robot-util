#ifndef MAP_H
#define MAP_H

#include <stddef.h>

struct map_callbacks {
    size_t (*hash_key)(void* key, size_t capacity, void* user_data);
    int (*keys_equal)(void* lhs, void* rhs, void* user_data);

    void* user_data;
};

typedef struct hashmap map_t;

typedef void (*map_iteration_callback_t)(void* key, void* value, void* user_data);

map_t* map_alloc_string_key(size_t capacity);

map_t* map_alloc(size_t capacity, const struct map_callbacks* callbacks);
void map_free(map_t* map);

void map_reserve(map_t* map, size_t capacity);

int map_key_exists(map_t* map, void* key);

size_t map_get_size(map_t* map);
size_t map_get_capacity(map_t* map);

int map_insert(map_t* map, void* key, void* value);
int map_remove(map_t* map, void* key);

int map_set(map_t* map, void* key, void* value);
int map_get(map_t* map, void* key, void** value);

void map_set_or_insert(map_t* map, void* key, void* value);

void map_iterate(map_t* map, map_iteration_callback_t callback, void* user_data);

#endif
