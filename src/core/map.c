#include "core/map.h"

#include "core/list.h"
#include "core/util.h"

#include <malloc.h>
#include <string.h>

typedef struct hashmap_node {
    void* key;
    void* value;

    struct hashmap_node* next;
} map_node_t;

struct hashmap {
    size_t capacity, size;

    // each bucket is a linked list
    map_node_t** buckets;

    list_t* valid_hashes;

    struct map_callbacks callbacks;
};

size_t map_hash_string(void* key, size_t capacity, void* user_data) {
    return util_hash_string((char*)key, capacity);
}

int map_strings_equal(void* lhs, void* rhs, void* user_data) {
    return strcmp((char*)lhs, (char*)rhs) == 0;
}

map_t* map_alloc_string_key(size_t capacity) {
    struct map_callbacks callbacks;

    callbacks.hash_key = map_hash_string;
    callbacks.keys_equal = map_strings_equal;

    callbacks.user_data = NULL;

    return map_alloc(capacity, &callbacks);
}

size_t map_default_hash(void* key, size_t capacity, void* user_data) {
    return (size_t)key % capacity;
}

int map_default_equality(void* lhs, void* rhs, void* user_data) { return lhs == rhs; }

map_t* map_alloc(size_t capacity, const struct map_callbacks* callbacks) {
    map_t* map;

    map = (map_t*)malloc(sizeof(map_t));
    memset(map, 0, sizeof(map_t));

    if (callbacks) {
        memcpy(&map->callbacks, callbacks, sizeof(struct map_callbacks));
    }

    map_reserve(map, capacity);
    return map;
}

void map_cleanup(map_t* map) {
    list_node_t* current_hash_node;

    size_t hash;
    map_node_t* current_bucket_node;
    map_node_t* next_bucket_node;

    while ((current_hash_node = list_begin(map->valid_hashes)) != NULL) {
        hash = (size_t)list_node_get(current_hash_node);

        current_bucket_node = map->buckets[hash];
        while (current_bucket_node) {
            next_bucket_node = current_bucket_node->next;
            free(current_bucket_node);

            current_bucket_node = next_bucket_node;
        }

        list_remove(map->valid_hashes, current_hash_node);
    }

    free(map->buckets);
    list_free(map->valid_hashes);
}

void map_free(map_t* map) {
    if (!map) {
        return;
    }

    map_cleanup(map);
    free(map);
}

void map_rehash_iterator(void* key, void* value, void* user_data) {
    map_t* new_map;

    new_map = (map_t*)user_data;
    map_insert(new_map, key, value);
}

void map_reserve(map_t* map, size_t capacity) {
    map_t temp;
    size_t list_size;

    if (capacity <= map->capacity) {
        return;
    }

    if (map->capacity > 0) {
        memcpy(&temp, map, sizeof(map_t));
    }

    list_size = capacity * sizeof(void*);
    map->buckets = (map_node_t**)malloc(list_size);
    memset(map->buckets, 0, list_size);

    map->capacity = capacity;
    map->size = 0;
    map->valid_hashes = list_alloc();

    if (temp.capacity > 0) {
        map_iterate(&temp, map_rehash_iterator, map);
        map_cleanup(&temp);
    }
}

size_t map_hash_key(map_t* map, void* key) {
    size_t hash;

    if (map->callbacks.hash_key) {
        hash = map->callbacks.hash_key(key, map->capacity, map->callbacks.user_data);
    } else {
        hash = (size_t)key;
    }

    hash %= map->capacity;

    return hash;
}

int map_keys_equal(map_t* map, void* lhs, void* rhs) {
    if (map->callbacks.keys_equal) {
        return map->callbacks.keys_equal(lhs, rhs, map->callbacks.user_data);
    } else {
        return lhs == rhs;
    }
}

map_node_t* map_find_node(map_t* map, void* key, size_t* node_hash) {
    size_t hash;
    map_node_t* current_node;

    hash = map_hash_key(map, key);
    current_node = map->buckets[hash];

    while (current_node) {
        if (map_keys_equal(map, current_node->key, key)) {
            if (node_hash) {
                *node_hash = hash;
            }

            return current_node;
        }

        current_node = current_node->next;
    }

    return NULL;
}

int map_key_exists(map_t* map, void* key) { return map_find_node(map, key, NULL) != NULL; }

size_t map_get_size(map_t* map) { return map->size; }
size_t map_get_capacity(map_t* map) { return map->capacity; }

int map_insert_with_hash(map_t* map, size_t hash, void* key, void* value, int check_duplicate) {
    map_node_t* current_node;
    map_node_t* new_node;

    list_node_t* hashes_end;

    if (check_duplicate) {
        current_node = map->buckets[hash];
        while (current_node) {
            if (map_keys_equal(map, current_node->key, key)) {
                return 0;
            }

            current_node = current_node->next;
        }
    }

    new_node = (map_node_t*)malloc(sizeof(map_node_t));
    new_node->key = key;
    new_node->value = value;

    new_node->next = map->buckets[hash];
    map->buckets[hash] = new_node;

    if (!new_node->next) {
        hashes_end = list_end(map->valid_hashes);
        list_insert(map->valid_hashes, hashes_end, (void*)hash);
    }

    return 1;
}

int map_insert(map_t* map, void* key, void* value) {
    size_t hash;

    hash = map_hash_key(map, value);
    return map_insert_with_hash(map, hash, key, value, 1);
}

int map_remove(map_t* map, void* key) {
    size_t hash;

    map_node_t* previous_node;
    map_node_t* current_node;

    hash = map_hash_key(map, key);

    previous_node = NULL;
    current_node = map->buckets[hash];

    while (current_node) {
        if (map_keys_equal(map, current_node->key, key)) {
            if (previous_node) {
                previous_node->next = current_node->next;
            } else {
                map->buckets[hash] = current_node->next;
            }

            free(current_node);
            return 1;
        }

        previous_node = current_node;
        current_node = current_node->next;
    }

    return 0;
}

int map_set(map_t* map, void* key, void* value) {
    map_node_t* node;

    node = map_find_node(map, key, NULL);
    if (!node) {
        return 0;
    }

    node->value = value;
    return 1;
}

int map_get(map_t* map, void* key, void** value) {
    map_node_t* node;

    node = map_find_node(map, key, NULL);
    if (!node) {
        return 0;
    }

    *value = node->value;
    return 1;
}

void map_set_or_insert(map_t* map, void* key, void* value) {
    map_node_t* node;
    size_t hash;

    node = map_find_node(map, key, &hash);
    if (node) {
        node->value = value;
    } else {
        map_insert_with_hash(map, hash, key, value, 0);
    }
}

void map_iterate(map_t* map, map_iteration_callback_t callback, void* user_data) {
    list_node_t* current_node;
    map_node_t* current_bucket_node;

    size_t hash;

    current_node = list_begin(map->valid_hashes);
    while (current_node) {
        hash = (size_t)list_node_get(current_node);
        current_bucket_node = map->buckets[hash];

        while (current_bucket_node) {
            callback(current_bucket_node->key, current_bucket_node->value, user_data);
            current_bucket_node = current_bucket_node->next;
        }

        current_node = list_node_next(current_node);
    }
}
