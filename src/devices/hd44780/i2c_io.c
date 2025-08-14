#include "devices/hd44780/screen.h"

#include "protocol/i2c.h"

#include "core/util.h"

#include <malloc.h>

typedef struct hd44780_i2c_io {
    i2c_device_t* device;
    int backlight_on;
} hd44780_i2c_io_t;

enum {
    HD44780_REGISTER_SELECT = (1 << 0),

    HD44780_ENABLE = (1 << 2),
    HD44780_BACKLIGHT_ON = (1 << 3),
};

int hd44780_i2c_write_nibble(hd44780_i2c_io_t* io, uint8_t nibble) {
    uint8_t value, backlight_flag;
    uint8_t buffer[2];
    int success;

    backlight_flag = io->backlight_on ? HD44780_BACKLIGHT_ON : 0;

    buffer[0] = nibble | HD44780_ENABLE | backlight_flag;
    buffer[1] = (nibble & ~HD44780_ENABLE) | backlight_flag;

    for (size_t i = 0; i < ARRAYSIZE(buffer); i++) {
        success = i2c_device_write(io->device, &buffer[i], sizeof(uint8_t));
        if (!success) {
            return 0;
        }
    }

    return success;
}

int hd44780_i2c_write_byte(hd44780_i2c_io_t* io, uint8_t byte, uint8_t flags) {
    int success;

    uint8_t nibbles[] = {
        (byte & 0xf0) | flags,
        ((byte & 0x0f) << 4) | flags,
    };

    for (size_t i = 0; i < ARRAYSIZE(nibbles); i++) {
        success = hd44780_i2c_write_nibble(io, nibbles[i]);
        if (!success) {
            return 0;
        }
    }

    return 1;
}

int hd44780_i2c_send_command(void* user_data, uint8_t command) {
    hd44780_i2c_io_t* io;

    io = (hd44780_i2c_io_t*)user_data;
    return hd44780_i2c_write_byte(io, command, 0);
}

int hd44780_i2c_send_data(void* user_data, const void* data, size_t size) {
    int success;

    hd44780_i2c_io_t* io;
    const uint8_t* bytes;
    
    io = (hd44780_i2c_io_t*)user_data;
    bytes = (const uint8_t*)data;

    for (size_t i = 0; i < size; i++) {
        success = hd44780_i2c_write_byte(io, bytes[i], HD44780_REGISTER_SELECT);
        if (!success) {
            return 0;
        }
    }

    return 1;
}

void hd44780_i2c_set_backlight(void* user_data, int backlight_on) {
    hd44780_i2c_io_t* io;

    io = (hd44780_i2c_io_t*)user_data;
    io->backlight_on = backlight_on;
}

int hd44780_i2c_init(void* user_data) {
    // resets the display
    static const uint8_t init_commands[] = { 0x03, 0x03, 0x03, 0x02 };

    hd44780_i2c_io_t* io;
    int success;

    io = (hd44780_i2c_io_t*)user_data;
    io->backlight_on = 1;

    for (size_t i = 0; i < ARRAYSIZE(init_commands); i++) {
        success = hd44780_i2c_send_command(io, init_commands[i]);
        if (!success) {
            return 0;
        }

        util_sleep_ms(1);
    }

    return 1;
}

void hd44780_i2c_close(void* user_data) {
    hd44780_i2c_io_t* io;

    io = (hd44780_i2c_io_t*)user_data;

    // todo: free things

    free(io);
}

hd44780_io_t* hd44780_i2c_open(i2c_device_t* device) {
    hd44780_i2c_io_t* data;
    hd44780_io_t* io;

    data = (hd44780_i2c_io_t*)malloc(sizeof(hd44780_i2c_io_t));
    data->device = device;

    io = (hd44780_io_t*)malloc(sizeof(hd44780_io_t));
    io->user_data = data;

    io->send_command = hd44780_i2c_send_command;
    io->send_data = hd44780_i2c_send_data;
    io->set_backlight = hd44780_i2c_set_backlight;

    io->io_init = hd44780_i2c_init;
    io->io_close = hd44780_i2c_close;

    return io;
}
