#ifndef HD44780_SCREEN_H
#define HD44780_SCREEN_H

#include <stdint.h>
#include <stddef.h>

typedef struct hd44780_io {
    // called from hd44780_open. return 1 on success, 0 on failure. can be null
    int (*io_init)(void* user_data);

    // called from hd44780_close. can be null
    void (*io_close)(void* user_data);

    // must be implemented. returns 1 on success, 0 on failure
    int (*send_command)(void* user_data, uint8_t command);

    // must be implemented. returns 1 on success, 0 on failure
    int (*send_data)(void* user_data, const void* data, size_t size);

    // can be null
    void (*set_backlight)(void* user_data, int backlight_on);

    void* user_data;
} hd44780_io_t;

struct hd44780_screen_config {
    // turns the display on
    int display_on;

    // enables an underline cursor
    int underline_cursor_visible;

    // enables a blinking cursor
    int blinking_cursor_visible;

    // sets the screen to shift rather than the cursor
    int auto_shift;

    // move the cursor on each character write
    int increment;

    // turn on the LCD backlight
    int backlight_on;
};

typedef struct hd44780 hd44780_t;

// opens an HD44780 screen. takes ownership of the IO interface
hd44780_t* hd44780_open(hd44780_io_t* io, const uint8_t* row_offsets, uint8_t width,
                        uint8_t height);

// closes the screen
void hd44780_close(hd44780_t* screen);

// retrieves the logical size of the screen
void hd44780_get_size(hd44780_t* screen, uint8_t* width, uint8_t* height);

// write text to the screen. returns 1 on success, 0 on failure
int hd44780_write(hd44780_t* screen, const char* text);

// clears the screen. returns 1 on success, 0 on failure
int hd44780_clear(hd44780_t* screen);

// returns to the "home" position of the screen (0, 0). returns 1 on success, 0 on failure
int hd44780_home(hd44780_t* screen);

// moves the cursor to the desired position, starting from the top-left. returns 1 on success, 0 on
// failure
int hd44780_set_cursor_pos(hd44780_t* screen, uint8_t x, uint8_t y);

// applies a config to the screen. returns 1 on success, 0 on failure
int hd44780_apply_config(hd44780_t* screen, const struct hd44780_screen_config* config);

// retrieves the last-applied configuration
void hd44780_get_config(hd44780_t* screen, struct hd44780_screen_config* config);

// SPECIALIZATION

// opens an HD44780 screen with dimensions 20x4. see hd44780_open
hd44780_t* hd44780_open_20x4(hd44780_io_t* io);

// IO INTERFACES

// from i2c.h
typedef struct i2c_device i2c_device_t;

// opens i2c interface for an expansion chip. does not assume ownership of device
hd44780_io_t* hd44780_i2c_open(i2c_device_t* device);

#endif
