#ifndef LIST_H
#define LIST_H

typedef struct linked_list list_t;
typedef struct list_node list_node_t;

// allocates a double-linked list
list_t* list_alloc();

// frees a double-linked list
void list_free(list_t* list);

// retrieves the first element of the list
list_node_t* list_begin(list_t* list);

// retrieves the last element of the list
list_node_t* list_end(list_t* list);

// inserts a node into the linked list. if previous is null, will insert it at the beginning
list_node_t* list_insert(list_t* list, list_node_t* previous, void* value);

// removes and frees the node from the list
void list_remove(list_t* list, list_node_t* node);

// retrieves the next node after the current one
list_node_t* list_node_next(list_node_t* node);

// retrieves the previous node before the current one
list_node_t* list_node_previous(list_node_t* node);

// retrieves the value of a node
void* list_node_get(list_node_t* node);

// sets the value of a node
void list_node_set(list_node_t* node, void* value);

#endif
