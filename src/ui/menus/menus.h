#ifndef MENUS_H
#define MENUS_H

// from ui/menu.h
typedef struct menu menu_t;

// from ui/app.h
typedef struct app app_t;

// from core/config.h
struct robot_util_config;

// does not assume ownership of anything
menu_t* menus_main(const struct robot_util_config* config, app_t* app);

menu_t* menus_bluetooth(app_t* app);

#endif
