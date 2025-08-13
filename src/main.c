#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <malloc.h>

#include <pthread.h>

#include "gpio.h"
#include "i2c.h"
#include "util.h"

#include "menu.h"

#include "devices/rotary_encoder.h"
#include "devices/hd44780/screen.h"

gpio_chip_t* chip;
rotary_encoder_t* encoder;

i2c_bus_t* bus;
i2c_device_t* device;
hd44780_t* screen;

menu_t* menu;
int should_exit, redraw_menu;

pthread_mutex_t mutex;
int mutex_initialized;

void menu_item_test() { printf("Test! Hello!\n"); }

void menu_item_quit() {
    printf("Quitting...\n");

    should_exit = 1;
}

int init() {
    struct rotary_encoder_pins pins;
    struct hd44780_screen_config screen_config;

    hd44780_io_t* screen_io;

    size_t current_menu_item;
    char* name_buffer;
    size_t max_name_len;
    uint8_t width, height;

    chip = NULL;
    encoder = NULL;

    bus = NULL;
    device = NULL;
    screen = NULL;

    mutex_initialized = 0;
    menu = NULL;

    should_exit = 0;
    redraw_menu = 1;

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

    bus = i2c_bus_open(1, NULL);
    if (!bus) {
        return 1;
    }

    device = i2c_device_open(bus, 0x27);
    screen_io = hd44780_i2c_open(device);
    screen = hd44780_open_20x4(screen_io);

    if (!screen) {
        return 1;
    }

    hd44780_get_size(screen, &width, &height);
    max_name_len = width - 1;
    name_buffer = (char*)malloc((max_name_len + 1) * sizeof(char));

    if (pthread_mutex_init(&mutex, NULL)) {
        perror("pthread_mutex_init");
        return 1;
    }

    mutex_initialized = 1;
    menu = menu_create();

    for (current_menu_item = 0; current_menu_item < 4; current_menu_item++) {
        snprintf(name_buffer, max_name_len, "Test #%u", (uint32_t)(current_menu_item + 1));
        menu_add(menu, name_buffer, menu_item_test);
    }

    menu_add(menu, "Quit", menu_item_quit);

    free(name_buffer);
    return 0;
}

void time_diff(const struct timespec* t0, const struct timespec* t1, struct timespec* delta) {
    delta->tv_sec = t1->tv_sec - t0->tv_sec;
    delta->tv_nsec = t1->tv_nsec - t0->tv_nsec;

    if (t1->tv_nsec < t0->tv_nsec) {
        delta->tv_sec -= 1;
        delta->tv_nsec += 1e9;
    }
}

void* sample_thread(void* arg) {
    static const uint32_t sample_interval_us = 5e3;

    int button_pressed;
    int pressed, motion;
    int success;
    const char* menu_item_name;

    int should_break;

    struct timespec t0, t1, delta;
    uint32_t delta_us;

    button_pressed = 0;
    while (1) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);

        success = rotary_encoder_get(encoder, &pressed, &motion);
        if (!success) {
            printf("Failed to query rotary encoder; skipping sample\n");
            continue;
        }

        if (motion != 0) {
            if (motion > 0) {
                printf("Moved cursor clockwise\n");
            } else {
                printf("Moved cursor counter-clockwise\n");
            }

            pthread_mutex_lock(&mutex);
            menu_move_cursor(menu, motion > 0);
            redraw_menu = 1;
            pthread_mutex_unlock(&mutex);
        }

        // trigger on release
        if (!pressed && button_pressed) {
            pthread_mutex_lock(&mutex);
            menu_item_name = menu_get_current_item_name(menu);
            printf("Selected menu item: %s\n", menu_item_name ? menu_item_name : "<null>");

            menu_select(menu);
            pthread_mutex_unlock(&mutex);
        }

        button_pressed = pressed;

        pthread_mutex_lock(&mutex);
        should_break = should_exit;
        pthread_mutex_unlock(&mutex);

        if (should_break) {
            break;
        }

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        time_diff(&t0, &t1, &delta);

        if (delta.tv_sec == 0) {
            delta_us = delta.tv_nsec / 1e3;
            if (delta_us < sample_interval_us) {
                util_sleep_us(sample_interval_us - delta_us);
            }
        }
    }

    return NULL;
}

int render_menu() {
    const char** items;
    uint8_t* name_lengths;

    size_t item_count, cursor;
    uint8_t width, height;
    size_t current_item;
    size_t name_len, max_name_len;
    uint8_t selected_name_length;

    if (!hd44780_clear(screen)) {
        return 1;
    }

    hd44780_get_size(screen, &width, &height);

    max_name_len = width - 1;
    char name_buffer[max_name_len + 1];

    pthread_mutex_lock(&mutex);
    item_count = menu_get_menu_items(menu, height, NULL, NULL);

    items = (const char**)malloc(item_count * sizeof(void*));
    item_count = menu_get_menu_items(menu, height, items, &cursor);
    pthread_mutex_unlock(&mutex);

    name_lengths = (uint8_t*)malloc(item_count * sizeof(uint8_t));
    for (current_item = 0; current_item < item_count; current_item++) {
        if (!hd44780_set_cursor_pos(screen, 0, (uint8_t)current_item)) {
            free(items);
            free(name_lengths);

            return 1;
        }

        name_len = strlen(items[current_item]);
        if (name_len <= max_name_len) {
            strncpy(name_buffer, items[current_item], name_len);
        } else {
            char temp_name_buffer[name_len + 1];
            strncpy(temp_name_buffer, items[current_item], name_len);

            // -3 to allow for elipses
            temp_name_buffer[max_name_len - 3] = '\0';
            snprintf(name_buffer, max_name_len, "%s...", temp_name_buffer);

            name_len = max_name_len;
        }

        name_lengths[current_item] = (uint8_t)max_name_len;
        if (!hd44780_write(screen, name_buffer)) {
            free(items);
            free(name_lengths);

            return 1;
        }
    }

    selected_name_length = name_lengths[cursor];

    free(items);
    free(name_lengths);

    pthread_mutex_lock(&mutex);
    redraw_menu = 0;
    pthread_mutex_unlock(&mutex);

    return hd44780_set_cursor_pos(screen, selected_name_length, (uint8_t)cursor);
}

void shutdown() {
    menu_free(menu);

    if (mutex_initialized) {
        pthread_mutex_destroy(&mutex);
    }

    hd44780_close(screen);
    i2c_device_close(device);
    i2c_bus_close(bus);

    rotary_encoder_close(encoder);
    gpio_chip_close(chip);
}

int main(int argc, const char** argv) {
    int result, should_break, should_redraw;
    pthread_t sample_thread_handle;

    result = init();
    if (!result) {
        if (pthread_create(&sample_thread_handle, NULL, sample_thread, NULL)) {
            perror("pthread_create");
            return 1;
        }

        should_break = 0;
        should_redraw = 0;

        while (1) {
            if (should_redraw) {
                result = render_menu();
            }

            pthread_mutex_lock(&mutex);
            if (result) {
                should_exit = 1;
            }

            should_break = should_exit;
            should_redraw = redraw_menu;
            pthread_mutex_unlock(&mutex);

            if (should_break) {
                break;
            }

            util_sleep_ms(5);
        }
    }

    pthread_join(sample_thread_handle, NULL);

    shutdown();
    return result;
}
