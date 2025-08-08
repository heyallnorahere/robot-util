#include "rotary_encoder.h"

#include "../gpio.h"

#include <malloc.h>
#include <string.h>

struct rotary_encoder {
    gpio_chip_t* chip;
    struct rotary_encoder_pins pins;

    int last_state;
};

rotary_encoder_t* rotary_encoder_open(gpio_chip_t* chip, const struct rotary_encoder_pins* pins) {
    rotary_encoder_t* encoder;
    struct gpio_request_config config;

    encoder = (rotary_encoder_t*)malloc(sizeof(rotary_encoder_t));
    encoder->chip = chip;

    memcpy(&encoder->pins, pins, sizeof(struct rotary_encoder_pins));

    config.type = GPIO_REQUEST_DIRECTION_INPUT;
    config.flags = GPIO_REQUEST_FLAG_BIAS_PULL_DOWN;

    // rotary_encoder_pins is packed
    if (!gpio_set_pin_request(chip, 3, (unsigned int*)pins, &config)) {
        free(encoder);
        return NULL;
    }

    if (!gpio_get_digital(chip, 1, &encoder->pins.a, &encoder->last_state)) {
        free(encoder);
        return NULL;
    }

    return encoder;
}

void rotary_encoder_close(rotary_encoder_t* encoder) {
    if (!encoder) {
        return;
    }

    free(encoder);
}

int rotary_encoder_get_direction(int a, int b, int last_state) {
    // we want to make sure a is high
    // so that we dont get duplicate signals
    if (a == last_state || !a) {
        return 0;
    }

    // when rotating clockwise, a rises before b
    if (a != b) {
        return 1;
    } else {
        return -1;
    }
}

int rotary_encoder_get_motion(rotary_encoder_t* encoder, int* motion) {
    int values[2];
    int a, b;

    unsigned int pins[2];
    int success;

    pins[0] = encoder->pins.a;
    pins[1] = encoder->pins.b;

    success = gpio_get_digital(encoder->chip, 2, pins, values);
    if (!success) {
        return 0;
    }

    a = values[0];
    b = values[1];

    *motion = rotary_encoder_get_direction(a, b, encoder->last_state);
    encoder->last_state = a;

    return 1;
}

int rotary_encoder_get(rotary_encoder_t* encoder, int* pressed, int* motion) {
    int success;

    success = 1;

    if (motion) {
        success &= rotary_encoder_get_motion(encoder, motion);
    }

    if (pressed) {
        success &= gpio_get_digital(encoder->chip, 1, &encoder->pins.sw, pressed);
    }

    return success;
}
