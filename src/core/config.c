#include "core/config.h"

#include "core/util.h"

#include <cjson/cJSON.h>

#include <malloc.h>
#include <string.h>

#include <sys/stat.h>

void config_default(struct robot_util_config* config) {
    config->lcd_address = 0x27;

    config->encoder_pins.a = 17;
    config->encoder_pins.b = 27;
    config->encoder_pins.sw = 22;

    config->update_url = NULL;
}

struct pin_mapping {
    const char* name;
    unsigned int* ptr;
};

int config_deserialize_encoder_pins(const cJSON* json, struct rotary_encoder_pins* pins) {
    struct pin_mapping mapping[] = {
        { "a", &pins->a },
        { "b", &pins->b },
        { "sw", &pins->sw },
    };

    const cJSON* node;
    const char* node_name;
    size_t i;

    for (i = 0; i < ARRAYSIZE(mapping); i++) {
        node_name = mapping[i].name;
        node = cJSON_GetObjectItemCaseSensitive(json, node_name);

        if (!node || !cJSON_IsNumber(node)) {
            fprintf(stderr, "Pin mapping does not contain pin \"%s\"\n", node_name);
            return 0;
        }

        *mapping[i].ptr = (unsigned int)cJSON_GetNumberValue(node);
    }

    return 1;
}

int config_deserialize(const cJSON* json, struct robot_util_config* config) {
    const cJSON* node;
    const char* node_name;
    size_t len;

    node_name = "lcd_address";
    node = cJSON_GetObjectItemCaseSensitive(json, node_name);

    if (!node || !cJSON_IsNumber(node)) {
        fprintf(stderr, "Config does not contain an I2C address (%s) for the LCD display!\n",
                node_name);

        return 0;
    }

    config->lcd_address = (uint16_t)cJSON_GetNumberValue(node);

    node_name = "encoder_pins";
    node = cJSON_GetObjectItemCaseSensitive(json, node_name);

    if (!node || !cJSON_IsObject(node)) {
        fprintf(stderr, "Config does not contain pins (%s) for a rotary encoder!\n", node_name);
        return 0;
    }

    if (!config_deserialize_encoder_pins(node, &config->encoder_pins)) {
        return 0;
    }

    node_name = "update_url";
    node = cJSON_GetObjectItemCaseSensitive(json, node_name);

    if (node && cJSON_IsString(node)) {
        len = strlen(node->valuestring);
        config->update_url = (char*)malloc((len + 1) * sizeof(char));

        strncpy(config->update_url, node->valuestring, len);
    } else {
        config->update_url = NULL;
    }

    return 1;
}

cJSON* config_serialize_encoder_pins(const struct rotary_encoder_pins* pins) {
    cJSON* node;

    node = cJSON_CreateObject();
    if (!node) {
        return NULL;
    }

    cJSON_AddNumberToObject(node, "a", pins->a);
    cJSON_AddNumberToObject(node, "b", pins->b);
    cJSON_AddNumberToObject(node, "sw", pins->sw);

    return node;
}

cJSON* config_serialize(const struct robot_util_config* config) {
    cJSON* config_node;
    cJSON* child;

    config_node = cJSON_CreateObject();
    if (!config_node) {
        return NULL;
    }

    child = config_serialize_encoder_pins(&config->encoder_pins);
    if (!child) {
        cJSON_Delete(config_node);
        return NULL;
    }

    cJSON_AddNumberToObject(config_node, "lcd_address", config->lcd_address);
    cJSON_AddItemToObject(config_node, "encoder_pins", child);

    if (config->update_url) {
        child = cJSON_CreateString(config->update_url);
    } else {
        child = cJSON_CreateNull();
    }

    cJSON_AddItemToObject(config_node, "update_url", child);

    return config_node;
}

int config_load(const char* path, struct robot_util_config* config) {
    char* config_buffer;
    size_t config_length;

    cJSON* json;
    const char* error;

    int success;

    config_buffer = (char*)util_read_file(path, &config_length);
    if (!config_buffer) {
        fprintf(stderr, "Failed to read config file: %s\n", path);
        return 0;
    }

    json = cJSON_ParseWithLength(config_buffer, config_length);
    free(config_buffer);

    if (!json) {
        error = cJSON_GetErrorPtr();
        if (error) {
            fprintf(stderr, "Failed to parse JSON config: %s\n", error);
        }

        return 0;
    }

    success = config_deserialize(json, config);

    cJSON_Delete(json);
    return success;
}

int config_save(const char* path, const struct robot_util_config* config) {
    char* buffer;
    const char* error;

    cJSON* json;
    size_t json_length;

    int status;
    ssize_t bytes_written;

    json = config_serialize(config);
    if (!json) {
        error = cJSON_GetErrorPtr();
        fprintf(stderr, "Failed to serialize config: %s\n", error ? error : "<null>");
        return 0;
    }

    buffer = util_get_dirname(path);
    if (buffer) {
        status = util_mkdir_recursive(buffer, S_IRWXU | S_IRWXG);
        free(buffer);

        if (status != 0) {
            perror("mkdir");

            cJSON_Delete(json);
            return 0;
        }
    }

    buffer = cJSON_Print(json);
    cJSON_Delete(json);

    json_length = strlen(buffer);
    bytes_written = util_write_file(path, buffer, json_length * sizeof(char));

    cJSON_free(buffer);
    return bytes_written == json_length;
}

int config_load_or_default(const char* path, struct robot_util_config* config) {
    printf("Loading config from path: %s\n", path);

    if (config_load(path, config)) {
        return 1;
    }

    printf("Loading default config and saving it to disk\n");

    config_default(config);
    return config_save(path, config);
}

void config_destroy(struct robot_util_config* config) {
    if (config->update_url) {
        free(config->update_url);
    }
}
