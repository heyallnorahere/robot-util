#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "gpio.h"
#include "devices/rotary_encoder.h"

int loop(rotary_encoder_t* encoder) {
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

        usleep(1000000 / 5);
    }

    return 0;
}

int main(int argc, const char** argv) {
    struct rotary_encoder_pins pins;

    gpio_chip_t* chip;
    rotary_encoder_t* encoder;

    int result;

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

    result = loop(encoder);

    rotary_encoder_close(encoder);
    gpio_chip_close(chip);

    return result;
}
