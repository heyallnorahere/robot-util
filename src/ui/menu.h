#ifndef MENU_H
#pragma MENU_H

#include <stddef.h>
#include <stdint.h>

typedef struct menu menu_t;
typedef void (*menu_item_callback_t)(void* user_data, void* item_data);
typedef void (*menu_free_callback_t)(void* user_data);

menu_t* menu_create();
void menu_free(menu_t* menu);

void menu_set_user_data(menu_t* menu, void* user_data, menu_free_callback_t free_callback);

void menu_add(menu_t* menu, const char* text, menu_item_callback_t action, void* user_data,
              menu_item_callback_t free_callback);

void menu_clear(menu_t* menu);

const char* menu_get_current_item_name(menu_t* menu);
size_t menu_get_menu_items(menu_t* menu, size_t max_items, const char** items, size_t* cursor);

void menu_move_cursor(menu_t* menu, int clockwise);
void menu_select(menu_t* menu);

#endif
