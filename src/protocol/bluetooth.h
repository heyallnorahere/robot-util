#ifndef BLUETOOTH_H
#define BLUETOOTH_H

typedef struct bluetooth bluetooth_t;
typedef struct bluetooth_agent bluetooth_agent_t;

bluetooth_t* bluetooth_connect(const char* interface);
void bluetooth_disconnect(bluetooth_t* bt);

bluetooth_agent_t* bluetooth_agent_create();
void bluetooth_agent_free(bluetooth_agent_t* agent);

void bluetooth_register_agent(bluetooth_t* bt, bluetooth_agent_t* agent);

#endif
