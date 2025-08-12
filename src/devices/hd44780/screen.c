#include "screen.h"

#include "../../util.h"

#include <string.h>
#include <malloc.h>

struct hd44780 {
    hd44780_io_t* io;

    size_t row_count;
    uint8_t* row_offsets;

    uint8_t display_mode;
    uint8_t display_control;
    uint8_t display_function;
};

// commands
enum {
    HD44780_CLEAR_DISPLAY = (1 << 0),
    HD44780_RETURN_HOME = (1 << 1),
    HD44780_SET_CG_RAM_ADDRESS = (1 << 6),
    HD44780_SET_DD_RAM_ADDRESS = (1 << 7),
};

// display mode
enum {
    HD44780_DISPLAY_MODE_COMMAND = (1 << 2),

    HD44780_DISPLAY_MODE_DISPLAY_SHIFT = (1 << 0),
    HD44780_DISPLAY_MODE_INCREMENT = (1 << 1),
};

// display control
enum {
    HD44780_DISPLAY_CONTROL_COMMAND = (1 << 3),

    HD44780_DISPLAY_CONTROL_BLINK_ON = (1 << 0),
    HD44780_DISPLAY_CONTROL_CURSOR_ON = (1 << 1),
    HD44780_DISPLAY_CONTROL_DISPLAY_ON = (1 << 2),
};

// display function
enum {
    HD44780_DISPLAY_FUNCTION_COMMAND = (1 << 5),

    HD44780_DISPLAY_FUNCTION_EXTENDED_INSTRUCTION_SET = (1 << 0),
    HD44780_DISPLAY_MODE_FONT_5_X_10 = (1 << 2),
    HD44780_DISPLAY_MODE_TWO_LINE = (1 << 3),
    HD44780_DISPLAY_MODE_EIGHT_BIT = (1 << 4),
};

int hd44780_send_command(hd44780_t* screen, uint8_t command) {
    return screen->io->send_command(screen->io->user_data, command);
}

int hd44780_send_data(hd44780_t* screen, const void* data, size_t size) {
    return screen->io->send_data(screen->io->user_data, data, size);
}

int hd44780_init(hd44780_t* screen) {
    int success;

    if (screen->io->io_init) {
        success = screen->io->io_init(screen->io->user_data);

        if (!success) {
            return 0;
        }
    }

    if (screen->row_count > 1) {
        screen->display_mode |= HD44780_DISPLAY_MODE_TWO_LINE;
    }

    screen->display_control |= HD44780_DISPLAY_CONTROL_DISPLAY_ON;
    screen->display_mode |= HD44780_DISPLAY_MODE_INCREMENT;

    uint8_t init_commands[] = {
        screen->display_function,
        screen->display_control,
        screen->display_mode,

        HD44780_CLEAR_DISPLAY,
    };

    for (size_t i = 0; i < ARRAYSIZE(init_commands); i++) {
        success = hd44780_send_command(screen, init_commands[i]);

        if (!success) {
            return 0;
        }
    }

    return 1;
}

hd44780_t* hd44780_open(hd44780_io_t* io, const uint8_t* row_offsets, size_t row_count) {
    hd44780_t* screen;
    int success;

    if (!io) {
        return NULL;
    }

    screen = (hd44780_t*)malloc(sizeof(hd44780_t*));
    screen->io = io;

    screen->row_count = row_count;
    screen->row_offsets = (uint8_t*)malloc(row_count * sizeof(uint8_t));
    memcpy(screen->row_offsets, row_offsets, row_count * sizeof(uint8_t));

    screen->display_mode = HD44780_DISPLAY_MODE_COMMAND;
    screen->display_control = HD44780_DISPLAY_CONTROL_COMMAND;
    screen->display_function = HD44780_DISPLAY_FUNCTION_COMMAND;

    if (!hd44780_init(screen)) {
        hd44780_close(screen);
        return NULL;
    }

    return screen;
}

void hd44780_close(hd44780_t* screen) {
    if (!screen) {
        return;
    }

    if (screen->io->io_close) {
        screen->io->io_close(screen->io->user_data);
    }

    free(screen->io);
    free(screen->row_offsets);
    free(screen);
}

hd44780_t* hd44780_open_20x4(hd44780_io_t* io) {
    static const uint8_t row_offsets[] = { 0, 64, 20, 84 };

    return hd44780_open(io, row_offsets, 4);
}
