#include <stdio.h>

#include "gpio.h"

int main(int argc, const char** argv) {
    gpio_chip_t* chip;
    gpio_request_config_t config;

    unsigned int pin;
    int value;

    chip = gpio_chip_open("/dev/gpiochip0", "robot-util");
    if (!chip) {
        perror("Failed to open GPIO chip!");
        return 1;
    }

    config.type = GPIO_REQUEST_DIRECTION_INPUT;
    pin = 25;

    if (!gpio_set_pin_request(chip, 1, &pin, &config)) {
        perror("Failed to set pin request!");

        gpio_chip_close(chip);
        return 1;
    }

    if (!gpio_get_digital(chip, 1, &pin, &value)) {
        perror("Failed to retrieve pin value!");

        gpio_chip_close(chip);
        return 1;
    }

    printf("Pin %u: %d\n", pin, value);

    gpio_chip_close(chip);
    return 0;
}
