#include "ui/menus/menus.h"

#include "ui/menu.h"
#include "ui/app.h"

#include "core/config.h"

#include <malloc.h>

#include <stdio.h>

struct main_menu {
    struct robot_util_config* config;
    app_t* const* app;
};

app_t* main_menu_get_app(const struct main_menu* data) {
    if (!data->app) {
        return NULL;
    }

    return *data->app;
}

void main_menu_exit(void* user_data) {
    struct main_menu* data;
    app_t* app;

    data = (struct main_menu*)user_data;
    app = main_menu_get_app(data);

    if (app) {
        app_pop_menu(app);
    } else {
        fprintf(stderr, "No app attached to menu!");
    }
}

void free_main_menu(void* user_data) {
    struct main_menu* data;

    data = (struct main_menu*)user_data;
    
    config_destroy(data->config);
    free(data->config);

    free(data);
}

menu_t* menus_main(struct robot_util_config* config, app_t* const* app) {
    struct main_menu* data;
    menu_t* menu;

    data = (struct main_menu*)malloc(sizeof(struct main_menu));
    data->config = config;
    data->app = app;

    menu = menu_create();
    menu_set_user_data(menu, data, free_main_menu);

    menu_add(menu, "Exit", main_menu_exit);

    return menu;
}
