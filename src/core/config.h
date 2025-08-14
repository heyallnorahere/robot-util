#ifndef CONFIG_H
#define CONFIG_H

#include "devices/rotary_encoder.h"

#include <stdint.h>

struct robot_util_config {
    struct rotary_encoder_pins encoder_pins;
    uint16_t lcd_address;
};

// loads a config from disk. returns 1 on success, 0 on failure
int config_load(const char* path, struct robot_util_config* config);

// saves a config to disk. returns 1 on success, 0 on failure
int config_save(const char* path, const struct robot_util_config* config);

// loads a config from disk if possible. otherwise, loads a default config and saves it to disk.
// returns 1 on success, 0 on failure
int config_load_or_default(const char* path, struct robot_util_config* config);

#endif
