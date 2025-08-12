#ifndef MENU_H
#pragma MENU_H

#include <stddef.h>
#include <stdint.h>

typedef struct menu menu_t;
typedef void (*menu_callback_t)();

menu_t* menu_create();
void menu_free(menu_t* menu);

void menu_add(menu_t* menu, const char* text, menu_callback_t action);
void menu_clear(menu_t* menu);

const char* menu_get_current_item_name(menu_t* menu);
size_t menu_get_menu_items(menu_t* menu, size_t max_items, const char** items, size_t* cursor);

void menu_move_cursor(menu_t* menu, int clockwise);
void menu_select(menu_t* menu);

#endif
