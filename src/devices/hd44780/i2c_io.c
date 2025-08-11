#include "screen.h"

#include "../../i2c.h"
#include "../../util.h"

#include <malloc.h>

typedef struct hd44780_i2c_io {
    i2c_device_t* device;
    int backlight_on;
} hd44780_i2c_io_t;

enum {
    HD44780_ENABLE = (1 << 2),
};

void hd44780_i2c_write_nibble(hd44780_i2c_io_t* io, uint8_t nibble, uint8_t* buffer) {
    uint8_t value, backlight_flag;

    value = (nibble & 0x0f) << 4;
    backlight_flag = io->backlight_on ? (1 << 3) : 0;

    buffer[0] = value | HD44780_ENABLE | backlight_flag;
    buffer[1] = (value & ~HD44780_ENABLE) | backlight_flag;
}

int hd44780_i2c_send_command(void* user_data, uint8_t command) {
    hd44780_i2c_io_t* io;
    uint8_t buffer[2 * 2]; // 2 sent bytes per nibble * 2 nibbles per byte

    io = (hd44780_i2c_io_t*)user_data;
    hd44780_i2c_write_nibble(io, (command & 0xf0) >> 4, &buffer[0]);
    hd44780_i2c_write_nibble(io, command & 0x0f, &buffer[2]);

    return i2c_device_write(io->device, buffer, sizeof(uint8_t) * 4);
}

int hd44780_i2c_send_data(void* user_data, const void* data, size_t size) {
    static const size_t stride = 2 * 2;

    hd44780_i2c_io_t* io;
    const uint8_t* bytes;
    uint8_t buffer[stride * size];
    
    io = (hd44780_i2c_io_t*)user_data;
    bytes = (const uint8_t*)bytes;

    for (size_t i = 0; i < size; i++) {
        size_t offset = stride * i;
        size_t lsb_offset = offset + 2;

        uint8_t value = bytes[i];
        hd44780_i2c_write_nibble(io, (value & 0xf0) >> 4, buffer + offset);
        hd44780_i2c_write_nibble(io, value & 0x0f, buffer + lsb_offset);
    }

    return i2c_device_write(io->device, buffer, stride * size);
}

int hd44780_i2c_set_backlight(void* user_data, int backlight_on) {
    hd44780_i2c_io_t* io;

    io = (hd44780_i2c_io_t*)user_data;
    io->backlight_on = backlight_on;

    return hd44780_i2c_send_command(io, 0x00);
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
