#include "protocol/bluetooth.h"

#include "core/map.h"

#include <malloc.h>
#include <string.h>

#include <dbus/dbus.h>

struct bluetooth {
    DBusConnection* connection;
    char* interface;

    map_t* agents;
};

struct bluetooth_agent {
    // used as a set
    map_t* connections;
};

bluetooth_t* bluetooth_connect(const char* interface) {
    bluetooth_t* bt;
    DBusError error;

    bt = (bluetooth_t*)malloc(sizeof(bluetooth_t));
    bt->agents = map_alloc(100, NULL);
    bt->interface = strdup(interface);

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

void bluetooth_disconnect(bluetooth_t *bt) {
    if (!bt) {
        return;
    }

    if (bt->connection) {
        dbus_connection_unref(bt->connection);
    }

    map_free(bt->agents);
}
