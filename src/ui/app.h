#ifndef APP_H
#define APP_H

typedef struct rotary_encoder rotary_encoder_t;
typedef struct hd44780 hd44780_t;

typedef struct app app_t;
typedef struct menu menu_t;

// initializes a UI application. assumes ownership of encoder, screen, and menu
app_t* app_create(rotary_encoder_t* encoder, hd44780_t* screen, menu_t* main_menu);

// destroys the application
void app_destroy(app_t* app);

// one application tick. self-regulates ticks per second
void app_update(app_t* app);

// request app to exit
void app_request_exit(app_t* app);

// should app exit?
int app_should_exit(app_t* app);

// get return status of the app
int app_get_status(app_t* app);

// push menu onto stack. assumes ownership of menu
void app_push_menu(app_t* app, menu_t* menu);

// pop menu from stack. frees menu. returns 1 on success, 0 on failure
void app_pop_menu(app_t* app);

#endif
