#include "ui/app.h"

#include "core/list.h"
#include "core/util.h"

#include "core/config.h"

#include "ui/menu.h"
#include "ui/menus/menus.h"
#include "ui/backends/backends.h"

#include <malloc.h>
#include <string.h>

#include <time.h>

#define EMBEDDED_BACKEND_NAME "embedded"
#define CURSES_BACKEND_NAME "curses"

struct app {
    struct robot_util_config* config;

    app_backend_t* backend;

    list_t* menus;
    int should_redraw;

    int should_exit;
    int status;

    int button_pressed;
};

void app_backend_create(app_t* app) {
    const char* backend_name;
    app_backend_t* backend;

    backend_name = app->config->backend_name;
    if (!backend_name) {
        backend_name = EMBEDDED_BACKEND_NAME;
    }

    if (!strcmp(backend_name, EMBEDDED_BACKEND_NAME)) {
        app->backend = app_backend_embedded(app->config);
    } else if (!strcmp(backend_name, CURSES_BACKEND_NAME)) {
        app->backend = app_backend_curses();
    } else {
        fprintf(stderr, "Invalid backend name: %s\n", backend_name);
        app->backend = NULL;
    }
}

app_t* app_create(struct robot_util_config* config) {
    app_t* app;
    menu_t* menu;

    app = (app_t*)malloc(sizeof(app_t));
    app->config = config;

    app->menus = NULL;
    app->backend = NULL;

    app_backend_create(app);
    if (!app->backend) {
        fprintf(stderr, "Failed to create UI backend!\n");

        app_destroy(app);
        return NULL;
    }

    app->menus = list_alloc();
    app->should_redraw = 1;

    app->should_exit = 0;
    app->status = 0;

    menu = menus_main(config, app);
    if (!menu) {
        fprintf(stderr, "Failed to push main menu!\n");

        app_destroy(app);
        return NULL;
    }

    app_push_menu(app, menu);
    return app;
}

void app_destroy(app_t* app) {
    list_node_t* current_node;
    menu_t* menu;

    if (!app) {
        return;
    }

    if (app->menus) {
        while ((current_node = list_end(app->menus)) != NULL) {
            menu = (menu_t*)list_node_get(current_node);
            menu_free(menu);

            list_remove(app->menus, current_node);
        }

        list_free(app->menus);
    }

    if (app->backend) {
        if (app->backend->backend_destroy) {
            app->backend->backend_destroy(app->backend->data);
        }

        free(app->backend);
    }

    free(app);
}

char* app_build_menu_render_data(menu_t* menu, uint32_t width, uint32_t height,
                                 char cursor_character) {
    const char** items;
    size_t item_count, cursor;
    size_t current_item;
    size_t line_len, max_name_len;

    list_t* lines;
    list_node_t* current_line;
    char* current_line_data;

    size_t render_data_offset;
    size_t total_characters, render_data_size;
    char* render_data;

    max_name_len = width - 2;
    char line_buffer[width + 1];

    item_count = menu_get_menu_items(menu, height, NULL, NULL);

    items = (const char**)malloc(item_count * sizeof(void*));
    item_count = menu_get_menu_items(menu, height, items, &cursor);

    lines = list_alloc();
    total_characters = 0;

    for (current_item = 0; current_item < item_count; current_item++) {
        memset(line_buffer, 0, (width + 1) * sizeof(char));

        line_len = strlen(items[current_item]);
        if (line_len <= max_name_len) {
            strncpy(line_buffer, items[current_item], line_len);
        } else {
            char temp_name_buffer[line_len + 1];
            memset(temp_name_buffer, 0, sizeof(temp_name_buffer));

            strncpy(temp_name_buffer, items[current_item], line_len);

            // -3 to allow for elipses
            temp_name_buffer[max_name_len - 3] = '\0';
            snprintf(line_buffer, max_name_len + 1, "%s...", temp_name_buffer);

            line_len = max_name_len;
        }

        // space & cursor character
        if (current_item == cursor) {
            snprintf(line_buffer + line_len, width - line_len + 1, " %c", cursor_character);
            line_len += 2;
        }

        list_insert(lines, list_end(lines), strdup(line_buffer));
        total_characters += line_len + 1;
    }

    free(items);

    render_data_size = (total_characters + 1) * sizeof(char);
    render_data = (char*)malloc(render_data_size);
    memset(render_data, 0, render_data_size);

    render_data_offset = 0;
    for (current_line = list_begin(lines); current_line != NULL;
         current_line = list_node_next(current_line)) {
        current_line_data = (char*)list_node_get(current_line);
        strncpy(render_data + render_data_offset, current_line_data,
                total_characters - render_data_offset);

        render_data_offset += strlen(current_line_data) + 1;
    }

    return render_data;
}

void app_render_menu(app_t* app, menu_t* top) {
    uint32_t width, height;
    char cursor_character;
    char* render_data;

    if (!app->backend->backend_render) {
        return;
    }

    if (!app->backend->backend_get_cursor_character ||
        !app->backend->backend_get_cursor_character(app->backend->data, &cursor_character)) {
        cursor_character = '<';
    }

    app->backend->backend_get_screen_size(app->backend->data, &width, &height);
    render_data = app_build_menu_render_data(top, width, height, cursor_character);

    app->backend->backend_render(app->backend->data, app, render_data);
    free(render_data);
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
    menu_t* top;

    top = app_get_top(app);
    if (!top) {
        // nothing the user can do
        app->should_exit = 1;
        return;
    }

    if (app->backend->backend_update) {
        app->backend->backend_update(app->backend->data, app);

        if (app->should_exit) {
            return;
        }
    }

    top = app_get_top(app);
    if (!top) {
        app->should_exit = 1;
        return;
    }

    if (app->should_redraw) {
        app_render_menu(app, top);
        app->should_redraw = 0;
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

void app_request_exit(app_t* app, int status) {
    app->should_exit = 1;
    app->status = status;
}

int app_should_exit(app_t* app) { return app->should_exit; }
int app_get_status(app_t* app) { return app->status; }

void app_push_menu(app_t* app, menu_t* menu) {
    list_node_t* end;

    end = list_end(app->menus);
    list_insert(app->menus, end, menu);

    app->should_redraw = 1;
}

void app_pop_menu(app_t* app) {
    list_node_t* end;
    menu_t* menu;

    end = list_end(app->menus);
    if (!end) {
        return;
    }

    menu = list_node_get(end);
    list_remove(app->menus, end);

    app->should_redraw = 1;

    menu_free(menu);
}

void app_move_cursor(app_t* app, int32_t increment) {
    menu_t* top;
    int32_t i, count;
    int clockwise;

    top = app_get_top(app);
    if (!top) {
        return;
    }

    clockwise = increment > 0;
    count = increment < 0 ? -increment : increment;

    for (i = 0; i < count; i++) {
        menu_move_cursor(top, clockwise);
    }

    app->should_redraw |= increment != 0;
}

void app_select(app_t* app) {
    menu_t* top;

    top = app_get_top(app);
    if (!top) {
        return;
    }

    menu_select(top);
}

void app_get_screen_size(app_t* app, uint32_t* width, uint32_t* height) {
    app->backend->backend_get_screen_size(app->backend->data, width, height);
}
