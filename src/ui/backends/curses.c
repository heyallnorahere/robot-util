#include "ui/backends/backends.h"

#include "ui/app.h"

#include <locale.h>

#include <curses.h>

#include <malloc.h>
#include <string.h>

struct curses_backend_data {
    // todo: data
    uint32_t placeholder;
};

void curses_backend_free(void* data) {
    endwin();

    free(data);
}

void curses_backend_update(void* data, app_t* app) {
    int c;

    while ((c = getch()) != ERR) {
        switch (c) {
        case 'j':
            app_move_cursor(app, 1);
            break;
        case 'k':
            app_move_cursor(app, -1);
            break;
        case KEY_ENTER:
        case '\n':
            app_select(app);
            break;
        }
    }
}

void curses_backend_render(void* data, app_t* app, const char* render_data) {
    const char* line_data;
    size_t line;

    clear();

    line_data = render_data;
    line = 0;

    while (*line_data != '\0') {
        mvprintw((int)line, 0, "%s", line_data);

        line_data += strlen(line_data) + 1;
        line++;
    }
}

void curses_backend_get_screen_size(void* data, uint32_t* width, uint32_t* height) {
    uint32_t screen_width, screen_height;
    int terminal_width, terminal_height;

    getmaxyx(stdscr, terminal_height, terminal_width);

    // lock to 20x4
    if (width) {
        screen_width = 20;
        if (terminal_width < screen_width) {
            screen_width = (uint32_t)terminal_width;
        }

        *width = screen_width;
    }

    if (height) {
        screen_height = 4;
        if (terminal_height < screen_height) {
            screen_height = (uint32_t)terminal_height;
        }

        *height = screen_height;
    }
}

app_backend_t* app_backend_curses() {
    struct curses_backend_data* data;
    app_backend_t* backend;

    setlocale(LC_ALL, "");

    data = (struct curses_backend_data*)malloc(sizeof(struct curses_backend_data));

    initscr();
    cbreak();
    noecho();

    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    backend = (app_backend_t*)malloc(sizeof(app_backend_t));
    memset(backend, 0, sizeof(app_backend_t));

    backend->data = data;

    backend->backend_destroy = curses_backend_free;
    backend->backend_update = curses_backend_update;
    backend->backend_render = curses_backend_render;
    backend->backend_get_screen_size = curses_backend_get_screen_size;

    return backend;
}
