#include "protocol/gpio.h"
#include "protocol/i2c.h"

#include "core/config.h"

#include "ui/menu.h"
#include "ui/app.h"

#include "ui/menus/menus.h"

#include "devices/rotary_encoder.h"
#include "devices/hd44780/screen.h"

#include <malloc.h>

#include <stdio.h>

struct robot_util_config* config;

gpio_chip_t* chip;
i2c_bus_t* bus;
i2c_device_t* device;

app_t* app;

int init() {
    rotary_encoder_t* encoder;

    hd44780_io_t* screen_io;
    hd44780_t* screen;

    menu_t* menu;

    chip = NULL;
    bus = NULL;
    device = NULL;

    app = NULL;

    config = (struct robot_util_config*)malloc(sizeof(struct robot_util_config));
    if (!config_load_or_default("config/util.json", config)) {
        return 1;
    }

    chip = gpio_chip_open("/dev/gpiochip0", "robot-util");
    if (!chip) {
        return 1;
    }

    encoder = rotary_encoder_open(chip, &config->encoder_pins);
    if (!encoder) {
        return 1;
    }

    bus = i2c_bus_open(1, NULL);
    if (!bus) {
        rotary_encoder_close(encoder);
        return 1;
    }

    device = i2c_device_open(bus, config->lcd_address);
    screen_io = hd44780_i2c_open(device);
    screen = hd44780_open_20x4(screen_io);

    if (!screen) {
        rotary_encoder_close(encoder);
        return 1;
    }

    menu = menus_main(config, &app);
    app = app_create(encoder, screen, menu);
    config = NULL;

    return 0;
}

void shutdown() {
    struct hd44780_screen_config screen_config;

    app_destroy(app);

    i2c_device_close(device);
    i2c_bus_close(bus);
    gpio_chip_close(chip);

    if (config) {
        config_destroy(config);
        free(config);
    }
}

int main(int argc, const char** argv) {
    int result;

    result = init();
    if (!result) {
        while (!app_should_exit(app)) {
            app_update(app);
        }

        result = app_get_status(app);
    }

    shutdown();
    return result;
}
