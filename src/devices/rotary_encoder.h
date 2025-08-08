#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

struct rotary_encoder_pins {
    unsigned int a, b;
    unsigned int sw;
};

typedef struct gpio_chip gpio_chip_t;
typedef struct rotary_encoder rotary_encoder_t;

// open a rotary encoder. you own the memory
rotary_encoder_t* rotary_encoder_open(gpio_chip_t* chip, const struct rotary_encoder_pins* pins);

// close a rotary encoder
void rotary_encoder_close(rotary_encoder_t* encoder);

// get the state of a rotary encoder. "clicked" is 1 if the button is pressed down. moved is
// non-zero if the encoder was moved. positive for clockwise, negative for counter-clockwise
int rotary_encoder_get(rotary_encoder_t* encoder, int* pressed, int* motion);

#endif
