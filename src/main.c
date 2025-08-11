#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "gpio.h"
#include "i2c.h"

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

        usleep(1000000 / 200);
    }

    return 0;
}

void dump_i2c_addresses(uint32_t bus_index) {
    i2c_bus_t* bus;
    i2c_device_t* device;
    struct i2c_bus_config config;

    uint8_t read_byte;
    ssize_t bytes_read;

    config.addr_type = I2C_ADDRESS_7_BITS;
    config.delay_ms = 1;

    bus = i2c_bus_open(bus_index, &config);
    if (!bus) {
        return;
    }

    for (uint16_t i = 0; i < 0xFF; i++) {
        device = i2c_device_open(bus, i);

        bytes_read = i2c_device_read(device, 0x0, &read_byte, 1);
        if (bytes_read >= 0) {
            printf("%#2x\n", (int32_t)i);
        }

        i2c_device_close(device);
    }

    i2c_bus_close(bus);
}

int main(int argc, const char** argv) {
    dump_i2c_addresses(1);

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
