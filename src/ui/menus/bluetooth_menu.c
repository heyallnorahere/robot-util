#include "ui/menus/menus.h"

#include "ui/app.h"
#include "ui/menu.h"

#include <stdio.h>

#include <malloc.h>

#include <dbus/dbus.h>

struct bluetooth_menu {
    app_t* app;

    DBusConnection* connection;
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

    if (data->connection) {
        dbus_connection_unref(data->connection);
    }

    free(data);
}

menu_t* menus_bluetooth(app_t* app) {
    struct bluetooth_menu* data;
    menu_t* menu;

    DBusError error;

    data = (struct bluetooth_menu*)malloc(sizeof(struct bluetooth_menu));
    data->app = app;

    dbus_error_init(&error);
    data->connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

    if (!data->connection) {
        fprintf(stderr, "Error opening system bus: %s\n", error.message);

        dbus_error_free(&error);
        bluetooth_menu_free(data);

        return NULL;
    }

    menu = menu_create();
    menu_set_user_data(menu, data, bluetooth_menu_free);

    menu_add(menu, "Refresh", bluetooth_menu_refresh);

    menu_add(menu, "Back", bluetooth_menu_back);

    dbus_error_free(&error);
    return menu;
}
