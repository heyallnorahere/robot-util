#ifndef MENUS_H
#define MENUS_H

typedef struct menu menu_t;
typedef struct app app_t;

struct robot_util_config;

// assumes ownership of the config
// pass a pointer to the main reference to the app. this is used to manipulate menus
menu_t* menus_main(struct robot_util_config* config, app_t* const* app);

menu_t* menus_bluetooth(app_t* app);

#endif
