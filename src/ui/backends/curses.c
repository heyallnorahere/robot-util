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

app_backend_t* app_backend_curses() {
    struct curses_backend_data* data;
    app_backend_t* backend;

    setlocale(LC_ALL, "");

//    data = (struct curses_backend_data*)malloc(sizeof(struct curses_backend_data));

    /*
    initscr();
    cbreak();
    noecho();

    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    */

    backend = (app_backend_t*)malloc(sizeof(app_backend_t));
    memset(backend, 0, sizeof(app_backend_t));

    return backend;
}
