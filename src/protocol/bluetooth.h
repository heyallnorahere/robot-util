#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>

typedef struct bluetooth bluetooth_t;
typedef struct bluetooth_agent bluetooth_agent_t;
typedef struct bluetooth_device bluetooth_device_t;

bluetooth_t* bluetooth_connect();
void bluetooth_disconnect(bluetooth_t* bt);

bluetooth_device_t** bluetooth_iterate_devices(bluetooth_t* bt, uint32_t* count);

char* bluetooth_device_get_name(bluetooth_device_t* device);
char* bluetooth_device_get_address(bluetooth_device_t* device);

int bluetooth_device_is_paired(bluetooth_device_t* device);

bluetooth_agent_t* bluetooth_agent_create(bluetooth_t* connection, const char* path);
void bluetooth_agent_free(bluetooth_agent_t* agent);

void bluetooth_register_agent(bluetooth_agent_t* agent);
void bluetooth_unregister_agent(bluetooth_agent_t* agent);

#endif
