#include "protocol/bluetooth.h"

#include "core/map.h"

#include <dbus/dbus.h>

struct bluetooth {
    DBusConnection* connection;

    map_t* agents;
};

struct bluetooth_agent {
    // used as a set
    map_t* connections;
};
