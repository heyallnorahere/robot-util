#ifndef APP_H
#define APP_H

#include <stdint.h>

// from core/config.h
struct robot_util_config;

// from ui/menu.h
typedef struct menu menu_t;

typedef struct app app_t;

typedef struct app_backend {
    void* data;

    // can be null. called on app_destroy
    void (*backend_destroy)(void* data);

    // can be null. update io. self-regulates ticks per second
    void (*backend_update)(void* data, app_t* app);

    // can be null. clear screen render data to it. each line is passed as multiple NUL-terminated
    // string laid out in sequence. the end of the data is denoted as an extra NUL character.
    void (*backend_render)(void* data, app_t* app, const char* render_data);

    // cannot be null. returns the size of the screen in characters via width and height pointers.
    // width and height can both be null.
    void (*backend_get_screen_size)(void* data, uint32_t* width, uint32_t* height);

    // can be null. return 1 if cursor_character was set
    int (*backend_get_cursor_character)(void* data, char* cursor_character);
} app_backend_t;

// initializes a UI application. assumes ownership of config.
app_t* app_create(struct robot_util_config* config);

// destroys the application
void app_destroy(app_t* app);

// one application tick. self-regulates ticks per second
void app_update(app_t* app);

// request app to exit
void app_request_exit(app_t* app, int status);

// should app exit?
int app_should_exit(app_t* app);

// get return status of the app
int app_get_status(app_t* app);

// push menu onto stack. assumes ownership of menu
void app_push_menu(app_t* app, menu_t* menu);

// pop menu from stack. frees menu. returns 1 on success, 0 on failure
void app_pop_menu(app_t* app);

// move the current menu's cursor
void app_move_cursor(app_t* app, int32_t increment);

// select the hovered menu item
void app_select(app_t* app);

// get the logical size of the screen in characters
void app_get_screen_size(app_t* app, uint32_t* width, uint32_t* height);

#endif
