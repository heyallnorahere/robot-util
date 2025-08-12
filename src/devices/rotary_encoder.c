#include "rotary_encoder.h"

#include "../gpio.h"

#include <malloc.h>
#include <string.h>

struct rotary_encoder_state {
    int a, b;
};

struct rotary_encoder {
    gpio_chip_t* chip;
    struct rotary_encoder_pins pins;

    struct rotary_encoder_state last_state;
};

int rotary_encoder_sample(rotary_encoder_t* encoder, struct rotary_encoder_state* state) {
    unsigned int pins[2];
    int values[2];

    int success;

    pins[0] = encoder->pins.a;
    pins[1] = encoder->pins.b;

    if (!gpio_get_digital(encoder->chip, 2, pins, values)) {
        return 0;
    }

    state->a = values[0];
    state->b = values[1];

    return 1;
}

rotary_encoder_t* rotary_encoder_open(gpio_chip_t* chip, const struct rotary_encoder_pins* pins) {
    rotary_encoder_t* encoder;
    struct gpio_request_config config;

    unsigned int rotary_pins[2];

    encoder = (rotary_encoder_t*)malloc(sizeof(rotary_encoder_t));
    encoder->chip = chip;

    memcpy(&encoder->pins, pins, sizeof(struct rotary_encoder_pins));

    config.type = GPIO_REQUEST_DIRECTION_INPUT;
    config.flags = GPIO_REQUEST_FLAG_BIAS_PULL_DOWN;

    rotary_pins[0] = pins->a;
    rotary_pins[1] = pins->b;

    if (!gpio_set_pin_request(chip, 2, rotary_pins, &config)) {
        rotary_encoder_close(encoder);
        return NULL;
    }

    config.flags = GPIO_REQUEST_FLAG_ACTIVE_LOW;

    if (!gpio_set_pin_request(chip, 1, &pins->sw, &config)) {
        rotary_encoder_close(encoder);
        return NULL;
    }

    if (!rotary_encoder_sample(encoder, &encoder->last_state)) {
        rotary_encoder_close(encoder);
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

int rotary_encoder_get_direction(const struct rotary_encoder_state* current,
                                 const struct rotary_encoder_state* last) {
    // we dont want duplicate signals
    if (current->a == last->a && current->b == last->b) {
        return 0;
    }

    if (current->a && !last->a && !current->b) {
        return 1;
    }

    if (current->b && !last->b && !current->a) {
        return -1;
    }

    return 0;
}

int rotary_encoder_get_motion(rotary_encoder_t* encoder, int* motion) {
    struct rotary_encoder_state state;
    if (!rotary_encoder_sample(encoder, &state)) {
        return 0;
    }

    *motion = rotary_encoder_get_direction(&state, &encoder->last_state);
    memcpy(&encoder->last_state, &state, sizeof(struct rotary_encoder_state));

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
