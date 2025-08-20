#include "ui/backends/backends.h"

#include "core/config.h"

#include "core/util.h"

#include "ui/app.h"

#include "protocol/gpio.h"
#include "protocol/i2c.h"

#include "devices/rotary_encoder.h"
#include "devices/hd44780/screen.h"

#include <malloc.h>
#include <string.h>

#include <time.h>

struct embedded_backend_data {
    gpio_chip_t* gpio_chip;
    i2c_bus_t* i2c_bus;

    rotary_encoder_t* encoder;

    i2c_device_t* screen_device;
    hd44780_t* screen;

    int button_pressed;

    struct timespec t0;
    int timestamp_valid;
};

int embedded_backend_dim_screen(hd44780_t* screen) {
    struct hd44780_screen_config screen_config;

    hd44780_get_config(screen, &screen_config);
    screen_config.backlight_on = 0;

    if (!hd44780_apply_config(screen, &screen_config)) {
        return 0;
    }

    return hd44780_clear(screen);
}

void embedded_backend_destroy(void* data) {
    struct embedded_backend_data* backend;

    backend = (struct embedded_backend_data*)data;

    if (backend->screen) {
        embedded_backend_dim_screen(backend->screen);
        hd44780_close(backend->screen);
    }

    i2c_device_close(backend->screen_device);

    rotary_encoder_close(backend->encoder);

    i2c_bus_close(backend->i2c_bus);
    gpio_chip_close(backend->gpio_chip);

    free(backend);
}

int embedded_backend_sample_encoder(struct embedded_backend_data* data, app_t* app) {
    int success;
    int pressed, motion;

    success = rotary_encoder_get(data->encoder, &pressed, &motion);
    if (!success) {
        return 0;
    }

    if (motion != 0) {
        app_move_cursor(app, (int32_t)motion);
    }

    // trigger on release
    if (!pressed && data->button_pressed) {
        app_select(app);
    }

    data->button_pressed = pressed;
    return 1;
}

void embedded_backend_update(void* data, app_t* app) {
    static const uint32_t tick_interval_us = 5e3; // 5ms

    struct embedded_backend_data* backend;

    struct timespec t1, delta;
    uint32_t delta_us;

    backend = (struct embedded_backend_data*)data;
    if (!embedded_backend_sample_encoder(backend, app)) {
        app_request_exit(app, 1);
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    if (backend->timestamp_valid) {
        util_time_diff(&backend->t0, &t1, &delta);

        if (delta.tv_sec == 0) {
            delta_us = delta.tv_nsec / 1e3;
            if (delta_us < tick_interval_us) {
                util_sleep_us(tick_interval_us - delta_us);
            }
        }
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &backend->t0);
    backend->timestamp_valid = 1;
}

void embedded_backend_render(void* data, app_t* app, const char* render_data) {
    struct embedded_backend_data* backend;

    const char* line_data;
    size_t line;

    backend = (struct embedded_backend_data*)data;

    if (!hd44780_clear(backend->screen)) {
        app_request_exit(app, 1);
        return;
    }

    line_data = render_data;
    line = 0;

    while (*line_data != '\0') {
        if (!hd44780_set_cursor_pos(backend->screen, 0, (uint8_t)line)) {
            app_request_exit(app, 1);
            return;
        }

        if (!hd44780_write(backend->screen, line_data)) {
            app_request_exit(app, 1);
            return;
        }

        line_data += strlen(line_data) + 1;
        line++;
    }
}

void embedded_backend_get_screen_size(void* data, uint32_t* width, uint32_t* height) {
    struct embedded_backend_data* backend;
    uint8_t screen_width, screen_height;

    backend = (struct embedded_backend_data*)data;
    hd44780_get_size(backend->screen, &screen_width, &screen_height);

    if (width) {
        *width = (uint32_t)screen_width;
    }

    if (height) {
        *height = (uint32_t)screen_height;
    }
}

app_backend_t* app_backend_embedded(const struct robot_util_config* config) {
    struct embedded_backend_data* data;
    app_backend_t* backend;

    hd44780_io_t* screen_io;

    data = (struct embedded_backend_data*)malloc(sizeof(struct embedded_backend_data));
    data->button_pressed = 0;
    data->timestamp_valid = 0;

    data->gpio_chip = NULL;
    data->i2c_bus = NULL;

    data->encoder = NULL;

    data->screen_device = NULL;
    data->screen = NULL;

    data->gpio_chip = gpio_chip_open("/dev/gpiochip0", "robot-util");
    if (!data->gpio_chip) {
        embedded_backend_destroy(data);
        return NULL;
    }

    data->i2c_bus = i2c_bus_open(1, NULL);
    if (!data->i2c_bus) {
        embedded_backend_destroy(data);
        return NULL;
    }

    data->encoder = rotary_encoder_open(data->gpio_chip, &config->encoder_pins);
    if (!data->encoder) {
        embedded_backend_destroy(data);
        return NULL;
    }

    data->screen_device = i2c_device_open(data->i2c_bus, config->lcd_address);
    screen_io = hd44780_i2c_open(data->screen_device);
    data->screen = hd44780_open_20x4(screen_io);

    if (!data->screen) {
        embedded_backend_destroy(data);
        return NULL;
    }

    backend = (app_backend_t*)malloc(sizeof(app_backend_t));
    backend->data = data;

    backend->backend_destroy = embedded_backend_destroy;
    backend->backend_update = embedded_backend_update;
    backend->backend_render = embedded_backend_render;
    backend->backend_get_screen_size = embedded_backend_get_screen_size;

    return backend;
}
