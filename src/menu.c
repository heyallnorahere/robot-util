#include "menu.h"

#include "list.h"

#include <malloc.h>
#include <string.h>

typedef struct menu_item {
    char* text;
    menu_callback_t action;
} menu_item_t;

struct menu {
    list_t* items;
    list_node_t* current_item;
};

menu_t* menu_create() {
    menu_t* menu;

    menu = (menu_t*)malloc(sizeof(menu_t));
    menu->items = list_alloc();
    menu->current_item = NULL;

    return menu;
}

void menu_free(menu_t* menu) {
    if (!menu) {
        return;
    }

    menu_clear(menu);

    list_free(menu->items);
    free(menu);
}

void menu_add(menu_t* menu, const char* text, menu_callback_t action) {
    menu_item_t* item;
    list_node_t* node;

    item = (menu_item_t*)malloc(sizeof(menu_item_t));
    item->text = strdup(text);
    item->action = action;

    node = list_insert(menu->items, list_end(menu->items), item);
    if (!menu->current_item) {
        menu->current_item = node;
    }
}

void menu_clear(menu_t* menu) {
    list_node_t* current_node;
    menu_item_t* item;

    current_node = list_begin(menu->items);
    do {
        item = (menu_item_t*)list_node_get(current_node);

        free(item->text);
        free(item);

        list_remove(menu->items, current_node);
        current_node = list_begin(menu->items);
    } while (current_node);
}

const char* menu_get_current_item_name(menu_t* menu) {
    menu_item_t* item;

    if (!menu->current_item) {
        return NULL;
    }

    item = (menu_item_t*)list_node_get(menu->current_item);
    return item->text;
}

size_t menu_get_menu_items(menu_t* menu, size_t max_items, const char** items, size_t* cursor) {
    list_node_t* current_node;
    size_t item_count, current_item, displayed_items;
    menu_item_t* item;

    current_node = list_begin(menu->items);
    item_count = 0;

    while (current_node) {
        item_count++;
        current_node = list_node_next(current_node);
    }

    displayed_items = item_count > max_items ? max_items : item_count;
    if (items) {
        if (item_count <= max_items || !menu->current_item) {
            current_item = 0;
            current_node = list_begin(menu->items);

            while (current_node) {
                item = (menu_item_t*)list_node_get(current_node);
                items[current_item] = item->text;

                if (cursor && menu->current_item == current_node) {
                    *cursor = current_item;
                }

                current_item++;
                current_node = list_node_next(current_node);
            }
        } else {
            if (cursor) {
                *cursor = 1;
            }

            current_node = list_node_previous(menu->current_item);
            if (!current_node) {
                current_node = list_end(menu->items);
            }

            for (current_item = 0; current_item < displayed_items; current_item++) {
                item = (menu_item_t*)list_node_get(current_node);
                items[current_item] = item->text;

                current_node = list_node_next(current_node);
                if (!current_node) {
                    current_node = list_begin(menu->items);
                }
            }
        }
    }

    return displayed_items;
}

void menu_move_cursor(menu_t* menu, int clockwise) {
    if (!menu->current_item) {
        return;
    }

    if (clockwise) {
        menu->current_item = list_node_next(menu->current_item);
        if (!menu->current_item) {
            menu->current_item = list_begin(menu->items);
        }
    } else {
        menu->current_item = list_node_previous(menu->current_item);
        if (!menu->current_item) {
            menu->current_item = list_end(menu->items);
        }
    }
}

void menu_select(menu_t* menu) {
    menu_item_t* item;

    if (!menu->current_item) {
        return;
    }

    item = (menu_item_t*)list_node_get(menu->current_item);
    item->action();
}
