#ifndef BLUETOOTH_H
#define BLUETOOTH_H

typedef struct bluetooth bluetooth_t;
typedef struct bluetooth_agent bluetooth_agent_t;
typedef struct bluetooth_device bluetooth_device_t;

bluetooth_t* bluetooth_connect();
void bluetooth_disconnect(bluetooth_t* bt);

bluetooth_agent_t* bluetooth_agent_create(bluetooth_t* connection, const char* path);
void bluetooth_agent_free(bluetooth_agent_t* agent);

void bluetooth_register_agent(bluetooth_agent_t* agent);
void bluetooth_unregister_agent(bluetooth_agent_t* agent);

#endif
