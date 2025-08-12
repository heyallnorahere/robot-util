#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "gpio.h"
#include "i2c.h"
#include "util.h"

#include "devices/rotary_encoder.h"
#include "devices/hd44780/screen.h"

gpio_chip_t* chip;
rotary_encoder_t* encoder;

i2c_bus_t* bus;
i2c_device_t* device;
hd44780_t* screen;

int init() {
    struct rotary_encoder_pins pins;
    hd44780_io_t* screen_io;

    chip = NULL;
    encoder = NULL;

    bus = NULL;
    device = NULL;
    screen = NULL;

    chip = gpio_chip_open("/dev/gpiochip0", "robot-util");
    if (!chip) {
        return 1;
    }

    pins.a = 17;
    pins.b = 27;
    pins.sw = 22;

    encoder = rotary_encoder_open(chip, &pins);
    if (!encoder) {
        return 1;
    }

    bus = i2c_bus_open(1, NULL);
    if (!bus) {
        return 1;
    }

    device = i2c_device_open(bus, 0x27);
    screen_io = hd44780_i2c_open(device);
    screen = hd44780_open_20x4(screen_io);

    if (!screen) {
        return 1;
    }

    return 0;
}

int loop() {
    int clicked, moved;
    int pressed, motion;
    int success;

    const char* motion_type;

    clicked = moved = 0;
    while (!clicked || !moved) {
        success = rotary_encoder_get(encoder, &pressed, &motion);
        if (!success) {
            return 1;
        }

        if (pressed && !clicked) {
            printf("Clicked!\n");
            clicked = 1;
        }

        if (motion != 0) {
            if (motion > 0) {
                motion_type = "clockwise";
            } else {
                motion_type = "counter-clockwise";
            }

            printf("Moved %s!\n", motion_type);
            moved = 1;
        }

        util_sleep_ms(5);
    }

    return 0;
}

void shutdown() {
    hd44780_close(screen);
    i2c_device_close(device);
    i2c_bus_close(bus);

    rotary_encoder_close(encoder);
    gpio_chip_close(chip);
}

int main(int argc, const char** argv) {
    int result;

    result = init();
    if (result == 0) {
        result = loop();
    }

    shutdown();
    return result;
}
