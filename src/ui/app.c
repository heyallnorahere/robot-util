#include "ui/app.h"

#include "core/list.h"
#include "core/util.h"

#include "devices/rotary_encoder.h"
#include "devices/hd44780/screen.h"

#include "ui/menu.h"

#include <malloc.h>
#include <string.h>

#include <time.h>

struct app {
    rotary_encoder_t* encoder;
    hd44780_t* screen;

    list_t* menus;
    int should_redraw;

    int should_exit;
    int status;

    int button_pressed;
};

app_t* app_create(rotary_encoder_t* encoder, hd44780_t* screen, menu_t* menu) {
    app_t* app;

    app = (app_t*)malloc(sizeof(app_t));
    app->encoder = encoder;
    app->screen = screen;

    app->menus = list_alloc();
    app->should_redraw = 1;

    app->should_exit = 0;
    app->status = 0;

    app_push_menu(app, menu);
    return app;
}

int app_dim_screen(hd44780_t* screen) {
    struct hd44780_screen_config screen_config;

    hd44780_get_config(screen, &screen_config);
    screen_config.backlight_on = 0;

    if (!hd44780_apply_config(screen, &screen_config)) {
        return 0;
    }

    return hd44780_clear(screen);
}

void app_destroy(app_t* app) {
    list_node_t* current_node;
    menu_t* menu;

    if (!app) {
        return;
    }

    while ((current_node = list_end(app->menus)) != NULL) {
        menu = (menu_t*)list_node_get(current_node);
        menu_free(menu);
    }

    if (app->screen) {
        // we dont care about the return value. were shutting down
        app_dim_screen(app->screen);
    }

    hd44780_close(app->screen);
    rotary_encoder_close(app->encoder);

    free(app);
}

int app_sample_encoder(app_t* app, menu_t* top) {
    const char* menu_item_name;

    int success;
    int pressed, motion;

    success = rotary_encoder_get(app->encoder, &pressed, &motion);
    if (!success) {
        return 0;
    }

    if (motion != 0) {
        if (motion > 0) {
            printf("Moved cursor clockwise\n");
        } else {
            printf("Moved cursor counter-clockwise\n");
        }

        menu_move_cursor(top, motion > 0);
        app->should_redraw = 1;
    }

    // trigger on release
    if (!pressed && app->button_pressed) {
        menu_item_name = menu_get_current_item_name(top);
        printf("Selected menu item: %s\n", menu_item_name ? menu_item_name : "<null>");

        menu_select(top);
    }

    app->button_pressed = pressed;
    return 1;
}

int app_render_menu(app_t* app, menu_t* top) {
    const char** items;
    size_t item_count, cursor;
    uint8_t width, height;
    size_t current_item;
    size_t name_len, max_name_len;

    if (!hd44780_clear(app->screen)) {
        return 0;
    }

    hd44780_get_size(app->screen, &width, &height);

    max_name_len = width - 2;
    char name_buffer[width + 1];

    item_count = menu_get_menu_items(top, height, NULL, NULL);

    items = (const char**)malloc(item_count * sizeof(void*));
    item_count = menu_get_menu_items(top, height, items, &cursor);

    for (current_item = 0; current_item < item_count; current_item++) {
        if (!hd44780_set_cursor_pos(app->screen, 0, (uint8_t)current_item)) {
            free(items);
            return 0;
        }

        memset(name_buffer, 0, (width + 1) * sizeof(char));

        name_len = strlen(items[current_item]);
        if (name_len <= max_name_len) {
            strncpy(name_buffer, items[current_item], name_len);
        } else {
            char temp_name_buffer[name_len + 1];
            strncpy(temp_name_buffer, items[current_item], name_len);

            // -3 to allow for elipses
            temp_name_buffer[max_name_len - 3] = '\0';
            snprintf(name_buffer, max_name_len, "%s...", temp_name_buffer);

            name_len = max_name_len;
        }

        if (current_item == cursor) {
            // space & arrow character
            strncpy(name_buffer + name_len, " \177", width - name_len);
        }

        if (!hd44780_write(app->screen, name_buffer)) {
            free(items);
            return 0;
        }
    }

    free(items);

    app->should_redraw = 0;
    return 1;
}

menu_t* app_get_top(app_t* app) {
    list_node_t* node;

    node = list_end(app->menus);
    if (!node) {
        return NULL;
    }

    return (menu_t*)list_node_get(node);
}

void app_update_menus(app_t* app) {
    int success;
    menu_t* top;

    top = app_get_top(app);
    if (!top) {
        // nothing the user can do
        app->should_exit = 1;
        return;
    }

    success = app_sample_encoder(app, top);
    if (success) {
        top = app_get_top(app);
        if (!top) {
            app->should_exit = 1;
            return;
        }

        if (app->should_redraw) {
            success = app_render_menu(app, top);
        }
    }

    if (!success) {
        app->status = 1;
        app->should_exit = 1;
    }
}

void app_update(app_t* app) {
    static const uint32_t tick_interval_us = 5e3;

    menu_t* top;

    struct timespec t0, t1, delta;
    uint32_t delta_us;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);

    app_update_menus(app);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    util_time_diff(&t0, &t1, &delta);

    if (delta.tv_sec == 0) {
        delta_us = delta.tv_nsec / 1e3;
        if (delta_us < tick_interval_us) {
            util_sleep_us(tick_interval_us - delta_us);
        }
    }
}

void app_request_exit(app_t* app) { app->should_exit = 1; }
int app_should_exit(app_t* app) { return app->should_exit; }

int app_get_status(app_t* app) { return app->status; }

void app_push_menu(app_t* app, menu_t* menu) {
    list_node_t* end;

    end = list_end(app->menus);
    list_insert(app->menus, end, menu);

    app->should_redraw = 1;
}

void app_pop_menu(app_t *app) {
    list_node_t* end;
    menu_t* menu;

    end = list_end(app->menus);
    if (!end) {
        return;
    }

    menu = list_node_get(end);
    list_remove(app->menus, end);

    menu_free(menu);
}
