#include "ui/menus/menus.h"

#include "ui/app.h"
#include "ui/menu.h"

#include "protocol/bluetooth.h"

#include <stdio.h>

#include <malloc.h>
#include <string.h>

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
    int paired;

    device = (bluetooth_device_t*)item_data;
    paired = bluetooth_device_is_paired(device);

    if (!paired) {
        bluetooth_device_pair(device);
    } else {
        bluetooth_device_remove(device);
    }

    bluetooth_menu_refresh(user_data, NULL);
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
    char* device_name;
    int device_paired;

    uint32_t screen_width;
    size_t name_buffer_size;
    char* name_buffer;

    data = (struct bluetooth_menu*)malloc(sizeof(struct bluetooth_menu));
    data->bt = bt;
    data->app = app;
    data->devices = bluetooth_iterate_devices(bt, &data->device_count);

    menu = menu_create();
    menu_set_user_data(menu, data, bluetooth_menu_free);

    menu_add(menu, "Refresh", bluetooth_menu_refresh, NULL, NULL);

    app_get_screen_size(app, &screen_width, NULL);
    name_buffer_size = (screen_width + 1) * sizeof(char);
    name_buffer = (char*)malloc(name_buffer_size);

    for (index = 0; index < data->device_count; index++) {
        device = data->devices[index];
        device_name = bluetooth_device_get_name(device);
        device_paired = bluetooth_device_is_paired(device);

        if (device_name) {
            memset(name_buffer, 0, name_buffer_size);
            snprintf(name_buffer, screen_width + 1, "%c%s", device_paired ? '*' : ' ', device_name);
            free(device_name);

            menu_add(menu, name_buffer, bluetooth_menu_select_device, device, NULL);
        }
    }

    free(name_buffer);

    menu_add(menu, "Back", bluetooth_menu_back, NULL, NULL);

    return menu;
}
