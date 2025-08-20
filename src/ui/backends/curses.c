#include "ui/backends/backends.h"

#include "ui/app.h"

#include <locale.h>

#include <curses.h>

#include <malloc.h>

struct curses_backend_data {
    // todo: data
    uint32_t placeholder;
};

void curses_backend_free(void* data) {
    endwin();

    free(data);
}

app_backend_t* app_backend_curses() {
    struct curses_backend_data* data;
    app_backend_t* backend;

    setlocale(LC_ALL, "");

    data = (struct curses_backend_data*)malloc(sizeof(struct curses_backend_data));

    if (!initscr()) {
        fprintf(stderr, "Failed to initialize ncurses!\n");

        free(data);
        return NULL;
    }

    cbreak();
    noecho();

    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
}
