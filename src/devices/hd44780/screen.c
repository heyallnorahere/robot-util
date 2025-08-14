#include "screen.h"

#include "../../util.h"

#include <string.h>
#include <malloc.h>

struct hd44780 {
    hd44780_io_t* io;

    uint8_t width, height;
    uint8_t* row_offsets;

    uint8_t display_mode;
    uint8_t display_control;
    uint8_t display_function;

    struct hd44780_screen_config current_config;
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
    HD44780_DISPLAY_FUNCTION_FONT_5_X_10 = (1 << 2),
    HD44780_DISPLAY_FUNCTION_TWO_LINE = (1 << 3),
    HD44780_DISPLAY_FUNCTION_EIGHT_BIT = (1 << 4),
};

int hd44780_send_command(hd44780_t* screen, uint8_t command) {
    return screen->io->send_command(screen->io->user_data, command);
}

int hd44780_send_data(hd44780_t* screen, const void* data, size_t size) {
    return screen->io->send_data(screen->io->user_data, data, size);
}

int hd44780_init(hd44780_t* screen) {
    int success;
    struct hd44780_screen_config config;

    if (screen->io->io_init) {
        success = screen->io->io_init(screen->io->user_data);

        if (!success) {
            return 0;
        }
    }

    if (screen->height > 1) {
        screen->display_function |= HD44780_DISPLAY_FUNCTION_TWO_LINE;
    }

    memset(&config, 0, sizeof(struct hd44780_screen_config));
    config.display_on = 1;
    config.backlight_on = 1;
    config.increment = 1;

    success = hd44780_apply_config(screen, &config);
    if (!success) {
        return 0;
    }

    success = hd44780_clear(screen);
    if (!success) {
        return 0;
    }

    return 1;
}

hd44780_t* hd44780_open(hd44780_io_t* io, const uint8_t* row_offsets, uint8_t width,
                        uint8_t height) {
    hd44780_t* screen;
    int success;

    if (!io) {
        return NULL;
    }

    screen = (hd44780_t*)malloc(sizeof(hd44780_t));
    screen->io = io;

    screen->width = width;
    screen->height = height;

    screen->row_offsets = (uint8_t*)malloc(height * sizeof(uint8_t));
    memcpy(screen->row_offsets, row_offsets, height * sizeof(uint8_t));

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

void hd44780_get_size(hd44780_t* screen, uint8_t* width, uint8_t* height) {
    if (width) {
        *width = screen->width;
    }

    if (height) {
        *height = screen->height;
    }
}

int hd44780_write(hd44780_t* screen, const char* text) {
    size_t data_length;

    // https://github.com/dotnet/iot/blob/main/src/devices/CharacterLcd/Hd44780.cs#L408
    // seems to be okay to just send the data
    // before testing nora thinks this wont work
    // well see

    data_length = strlen(text);
    return hd44780_send_data(screen, text, data_length);
}

int hd44780_clear(hd44780_t* screen) {
    if (!hd44780_send_command(screen, HD44780_CLEAR_DISPLAY)) {
        return 0;
    }

    util_sleep_ms(2);
    return 1;
}

int hd44780_home(hd44780_t* screen) {
    if (!hd44780_send_command(screen, HD44780_RETURN_HOME)) {
        return 0;
    }

    // documented as taking 1.52ms
    util_sleep_us(1520);

    return 1;
}

int hd44780_set_cursor_pos(hd44780_t* screen, uint8_t x, uint8_t y) {
    uint8_t address;

    if (x >= screen->width || y >= screen->height) {
        return 0;
    }

    address = x + screen->row_offsets[y];
    return hd44780_send_command(screen, HD44780_SET_DD_RAM_ADDRESS | address);
}

int hd44780_send_config(hd44780_t* screen) {
    int success;

    uint8_t init_commands[] = {
        screen->display_function,
        screen->display_control,
        screen->display_mode,
    };

    for (size_t i = 0; i < ARRAYSIZE(init_commands); i++) {
        success = hd44780_send_command(screen, init_commands[i]);

        if (!success) {
            return 0;
        }

        util_sleep_ms(1);
    }

    return 1;
}

int hd44780_apply_config(hd44780_t* screen, const struct hd44780_screen_config* config) {
    int success;

    util_set_bit_flag(&screen->display_control, HD44780_DISPLAY_CONTROL_DISPLAY_ON,
                      config->display_on);

    util_set_bit_flag(&screen->display_control, HD44780_DISPLAY_CONTROL_CURSOR_ON,
                      config->underline_cursor_visible);

    util_set_bit_flag(&screen->display_control, HD44780_DISPLAY_CONTROL_BLINK_ON,
                      config->blinking_cursor_visible);

    util_set_bit_flag(&screen->display_mode, HD44780_DISPLAY_MODE_DISPLAY_SHIFT,
                      config->auto_shift);

    util_set_bit_flag(&screen->display_mode, HD44780_DISPLAY_MODE_INCREMENT, config->increment);

    screen->io->set_backlight(screen->io->user_data, config->backlight_on);

    success = hd44780_send_config(screen);
    if (!success) {
        // revert
        screen->io->set_backlight(screen->io->user_data, screen->current_config.backlight_on);

        return 0;
    }

    memcpy(&screen->current_config, config, sizeof(struct hd44780_screen_config));
    return 1;
}

void hd44780_get_config(hd44780_t* screen, struct hd44780_screen_config* config) {
    memcpy(config, &screen->current_config, sizeof(struct hd44780_screen_config));
}

hd44780_t* hd44780_open_20x4(hd44780_io_t* io) {
    static const size_t width = 20;
    static const size_t height = 4;

    static const uint8_t row_offsets[] = { 0, 64, 20, 84 };

    return hd44780_open(io, row_offsets, width, height);
}
