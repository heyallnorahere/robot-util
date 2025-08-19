#include "protocol/bluetooth.h"

#include "core/map.h"

#include <stdint.h>

#include <malloc.h>

#include <dbus/dbus.h>
#include <gio/gio.h>

#include <pthread.h>

struct bluetooth {
    DBusConnection* connection;

    // maps string (path) to agent ptr
    map_t* agents;
};

struct bluetooth_agent {
    bluetooth_t* connection;
};

bluetooth_t* bluetooth_connect() {
    bluetooth_t* bt;
    DBusError error;

    bt = (bluetooth_t*)malloc(sizeof(bluetooth_t));
    bt->agents = map_alloc(100, NULL);

    dbus_error_init(&error);
    bt->connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

    if (!bt->connection) {
        fprintf(stderr, "Error retrieving system bus: %s\n", error.message);

        dbus_error_free(&error);
        bluetooth_disconnect(bt);

        return NULL;
    }

    dbus_error_free(&error);
    return bt;
}

void bluetooth_iterate_free_agent(void* key, void* value, void* user_data) {
    bluetooth_agent_free((bluetooth_agent_t*)value);
}

void bluetooth_disconnect(bluetooth_t* bt) {
    if (!bt) {
        return;
    }

    if (bt->connection) {
        dbus_connection_unref(bt->connection);
    }

    map_iterate(bt->agents, bluetooth_iterate_free_agent, bt);
    map_free(bt->agents);

    free(bt);
}

bluetooth_agent_t* bluetooth_agent_create(bluetooth_t* connection, const char* path) {
    bluetooth_agent_t* agent;

    agent = (bluetooth_agent_t*)malloc(sizeof(bluetooth_agent_t));
    agent->connection = connection;
}
