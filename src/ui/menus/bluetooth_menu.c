#include "ui/menus/menus.h"

#include "ui/app.h"
#include "ui/menu.h"

#include "protocol/bluetooth.h"

#include <stdio.h>

#include <malloc.h>

struct bluetooth_menu {
    bluetooth_t* bt;
    app_t* app;

    uint32_t device_count;
    bluetooth_device_t** devices;
};

void bluetooth_menu_refresh(void* user_data, void* item_data) {
    struct bluetooth_menu* data;

    bluetooth_t* bt;
    app_t* app;

    menu_t* menu;

    data = (struct bluetooth_menu*)user_data;

    // data is about to be freed by app_pop_menu
    bt = data->bt;
    app = data->app;

    app_pop_menu(app);

    menu = menus_bluetooth(bt, app);
    if (!menu) {
        fprintf(stderr, "Error while refreshing bluetooth device list\n");
    }

    app_push_menu(app, menu);
}

void bluetooth_menu_select_device(void* user_data, void* item_data) {
    bluetooth_device_t* device;
    const char* device_name;

    device = (bluetooth_device_t*)item_data;
    device_name = bluetooth_device_get_name(device);

    // todo: something!
}

void bluetooth_menu_back(void* user_data, void* item_data) {
    struct bluetooth_menu* data;

    data = (struct bluetooth_menu*)user_data;
    app_pop_menu(data->app);
}

void bluetooth_menu_free(void* user_data) {
    struct bluetooth_menu* data;

    data = (struct bluetooth_menu*)user_data;

    free(data);
}

menu_t* menus_bluetooth(bluetooth_t* bt, app_t* app) {
    struct bluetooth_menu* data;
    menu_t* menu;

    uint32_t index;
    bluetooth_device_t* device;
    const char* device_name;

    data = (struct bluetooth_menu*)malloc(sizeof(struct bluetooth_menu));
    data->bt = bt;
    data->app = app;
    data->devices = bluetooth_iterate_devices(bt, &data->device_count);

    menu = menu_create();
    menu_set_user_data(menu, data, bluetooth_menu_free);

    menu_add(menu, "Refresh", bluetooth_menu_refresh, NULL, NULL);
    for (index = 0; index < data->device_count; index++) {
        device = data->devices[index];
        device_name = bluetooth_device_get_name(device);

        if (device_name) {
            menu_add(menu, device_name, bluetooth_menu_select_device, device, NULL);
        }
    }

    menu_add(menu, "Back", bluetooth_menu_back, NULL, NULL);

    return menu;
}
