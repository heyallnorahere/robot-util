#ifndef HD44780_SCREEN_H
#define HD44780_SCREEN_H

#include <stdint.h>
#include <stddef.h>

typedef struct hd44780_io {
    // called from hd44780_open. return 1 on success, 0 on failure. can be null
    int(*io_init)(void* user_data);

    // called from hd44780_close. can be null
    void(*io_close)(void* user_data);

    // must be implemented. returns 1 on success, 0 on failure
    int(*send_command)(void* user_data, uint8_t command);

    // must be implemented. returns 1 on success, 0 on failure
    int(*send_data)(void* user_data, const void* data, size_t size);

    // can be null. returns 1 on success, 0 on failure
    int(*set_backlight)(void* user_data, int backlight_on);

    void* user_data;
} hd44780_io_t;

typedef struct hd44780 hd44780_t;

// opens an HD44780 screen. takes ownership of the IO interface
hd44780_t* hd44780_open(hd44780_io_t* io, const uint8_t* row_offsets, size_t row_count);

// closes the screen
void hd44780_close(hd44780_t* screen);

// SPECIALIZATION

// opens an HD44780 screen with dimensions 20x4. see hd44780_open
hd44780_t* hd44780_open_20x4(hd44780_io_t* io);

// IO INTERFACES

// from i2c.h
typedef struct i2c_device i2c_device_t;

// opens i2c interface for an expansion chip. does not assume ownership of device
hd44780_io_t* hd44780_i2c_open(i2c_device_t* device);

#endif
