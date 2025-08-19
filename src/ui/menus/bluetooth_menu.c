#include "ui/menus/menus.h"

#include "ui/app.h"
#include "ui/menu.h"

#include <stdio.h>

#include <malloc.h>

struct bluetooth_menu {
    app_t* app;
};

void bluetooth_menu_refresh(void* user_data) {
    struct bluetooth_menu* data;
    app_t* app;

    menu_t* menu;

    data = (struct bluetooth_menu*)user_data;
    app = data->app;

    app_pop_menu(app);

    menu = menus_bluetooth(app);
    if (!menu) {
        fprintf(stderr, "Error while refreshing bluetooth device list\n");
    }

    app_push_menu(app, menu);
}

void bluetooth_menu_back(void* user_data) {
    struct bluetooth_menu* data;

    data = (struct bluetooth_menu*)user_data;
    app_pop_menu(data->app);
}

void bluetooth_menu_free(void* user_data) {
    struct bluetooth_menu* data;

    data = (struct bluetooth_menu*)user_data;

    free(data);
}

menu_t* menus_bluetooth(app_t* app) {
    struct bluetooth_menu* data;
    menu_t* menu;

    data = (struct bluetooth_menu*)malloc(sizeof(struct bluetooth_menu));
    data->app = app;

    menu = menu_create();
    menu_set_user_data(menu, data, bluetooth_menu_free);

    menu_add(menu, "Refresh", bluetooth_menu_refresh);
    menu_add(menu, "Back", bluetooth_menu_back);

    return menu;
}
