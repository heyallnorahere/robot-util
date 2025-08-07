#include "gpio.h"

#include "util.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <gpiod.h>

// gpio_chip_t = struct gpio_chip
struct gpio_chip {
    struct gpiod_chip* chip;

    char* device;
    char* consumer;
};

gpio_chip_t* gpio_chip_open(const char* device, const char* consumer) {
    gpio_chip_t* chip;

    int error;
    int len;

    chip = (gpio_chip_t*)malloc(sizeof(gpio_chip_t));
    chip->device = util_copy_string(device);
    chip->consumer = util_copy_string(consumer);

    chip->chip = gpiod_chip_open(device);
    if (!chip->chip) {
        perror("gpiod_chip_open");

        gpio_chip_close(chip);
        return NULL;
    }

    return chip;
}

void gpio_chip_close(gpio_chip_t* chip) {
    if (!chip) {
        return;
    }

    if (chip->chip) {
        gpiod_chip_close(chip->chip);
    }

    free(chip->device);
    free(chip->consumer);

    free(chip);
}

int gpio_get_lines(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins,
                   struct gpiod_line_bulk* lines) {
    int error;

    unsigned int offsets[pin_count];
    memcpy(offsets, pins, pin_count * sizeof(unsigned int));

    error = gpiod_chip_get_lines(chip->chip, offsets, pin_count, lines);
    if (error) {
        perror("gpiod_chip_get_lines");
        return 0;
    }

    return 1;
}

int gpio_set_pin_request(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins,
                         const gpio_request_config_t* config) {
    struct gpiod_line_request_config gpiod_config;
    struct gpiod_line_bulk lines;

    int default_values[pin_count];
    int error;

    if (!gpio_get_lines(chip, pin_count, pins, &lines)) {
        return 0;
    }

    memset(&gpiod_config, 0, sizeof(struct gpiod_line_request_config));
    gpiod_config.consumer = chip->consumer;
    gpiod_config.request_type = (int)config->type;
    gpiod_config.flags = 0;

    memset(default_values, 0, sizeof(int) * pin_count);

    error = gpiod_line_request_bulk(&lines, &gpiod_config, default_values);
    if (error) {
        perror("gpiod_line_request_bulk");
        return 0;
    }

    return 1;
}

int gpio_set_digital(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins,
                     const int* values) {
    struct gpiod_line_bulk lines;

    int error;

    if (!gpio_get_lines(chip, pin_count, pins, &lines)) {
        return 0;
    }

    error = gpiod_line_set_value_bulk(&lines, values);
    if (error) {
        perror("gpiod_line_set_value_bulk");
        return 0;
    }

    return 1;
}

int gpio_get_digital(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins, int* values) {
    struct gpiod_line_bulk lines;

    int error;

    if (!gpio_get_lines(chip, pin_count, pins, &lines)) {
        return 0;
    }

    error = gpiod_line_get_value_bulk(&lines, values);
    if (error) {
        perror("gpiod_line_get_value_bulk");
        return 0;
    }
    
    return 1;
}
