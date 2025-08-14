#ifndef GPIO_H
#define GPIO_H

#include <stddef.h>
#include <stdint.h>

typedef struct gpio_chip gpio_chip_t;

typedef enum gpio_request_type {
    GPIO_REQUEST_DIRECTION_AS_IS = 1,
    GPIO_REQUEST_DIRECTION_INPUT,
    GPIO_REQUEST_DIRECTION_OUTPUT,
    GPIO_REQUEST_EVENT_FALLING_EDGE,
    GPIO_REQUEST_EVENT_RISING_EDGE,
    GPIO_REQUEST_EVENT_BOTH_EDGES,
} gpio_request_type;

typedef enum gpio_request_flag {
    GPIO_REQUEST_FLAG_OPEN_DRAIN = 1 << 0,
    GPIO_REQUEST_FLAG_OPEN_SOURCE = 1 << 1,
    GPIO_REQUEST_FLAG_ACTIVE_LOW = 1 << 2,
    GPIO_REQUEST_FLAG_BIAS_DISABLE = 1 << 3,
    GPIO_REQUEST_FLAG_BIAS_PULL_DOWN = 1 << 4,
    GPIO_REQUEST_FLAG_BIAS_PULL_UP = 1 << 5,
} gpio_request_flag;

struct gpio_request_config {
    gpio_request_type type;
    uint32_t flags;
};

// opens a gpio chip with a consistent consumer. copies strings.
gpio_chip_t* gpio_chip_open(const char* device, const char* consumer);

// closes gpio chip
void gpio_chip_close(gpio_chip_t* chip);

// sets the request of a set of pins
// return 1 on success, 0 on failure
int gpio_set_pin_request(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins,
                         const struct gpio_request_config* config);

// sets digital state of a set of pins
// returns 1 on success, 0 on failure
int gpio_set_digital(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins,
                     const int* values);

// gets digital state of a set of pins
// returns 1 on success, 0 on failure
int gpio_get_digital(gpio_chip_t* chip, size_t pin_count, const unsigned int* pins, int* values);

#endif
