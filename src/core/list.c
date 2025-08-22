#include "core/list.h"

#include <malloc.h>

struct linked_list {
    list_node_t* first;
    list_node_t* last;
};

struct list_node {
    void* value;

    list_node_t* next;
    list_node_t* previous;
};

list_t* list_alloc() {
    list_t* list;

    list = (list_t*)malloc(sizeof(list_t));
    list->first = NULL;
    list->last = NULL;

    return list;
}

void list_free(list_t* list) {
    if (!list) {
        return;
    }

    list_node_t* current = list->first;
    list_node_t* next;

    while (current) {
        next = current->next;
        free(current);

        current = next;
    }

    free(list);
}

list_node_t* list_begin(list_t* list) { return list->first; }
list_node_t* list_end(list_t* list) { return list->last; }

list_node_t* list_insert(list_t* list, list_node_t* previous, void* value) {
    list_node_t* node;

    node = (list_node_t*)malloc(sizeof(list_node_t));
    node->value = value;
    node->previous = previous;

    // link previous to current node
    if (previous) {
        node->next = previous->next;
        previous->next = node;
    } else {
        node->next = list->first;
        list->first = node;
    }

    // link next back to the current node
    if (node->next) {
        node->next->previous = node;
    } else {
        list->last = node;
    }

    return node;
}

void list_remove(list_t* list, list_node_t* node) {
    if (!node) {
        return;
    }

    // mend link from next to previous
    if (node->next) {
        node->next->previous = node->previous;
    } else {
        list->last = node->previous;
    }

    // mend link from previous to next
    if (node->previous) {
        node->previous->next = node->next;
    } else {
        list->first = node->next;
    }

    free(node);
}

void list_clear(list_t* list) {
    list_node_t* current;
    list_node_t* next;

    for (current = list->first; current != NULL; current = next) {
        next = current->next;
        free(current);
    }

    list->first = list->last = NULL;
}

list_node_t* list_node_next(list_node_t* node) { return node->next; }
list_node_t* list_node_previous(list_node_t* node) { return node->previous; }

void* list_node_get(list_node_t* node) { return node->value; }
void list_node_set(list_node_t* node, void* value) { node->value = value; }
