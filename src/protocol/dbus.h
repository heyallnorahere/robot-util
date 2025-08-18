#ifndef UTIL_DBUS_H
#define UTIL_DBUS_H

#include <stdint.h>

// from dbus/dbus.h
typedef struct DBusConnection DBusConnection;

typedef enum dbus_service_argument_direction {
    DBUS_SERVICE_ARGUMENT_IN,
    DBUS_SERVICE_ARGUMENT_OUT
} dbus_service_argument_direction;

struct dbus_service_argument_spec {
    const char* name;
    const char* type;
    dbus_service_argument_direction direction;
};

struct dbus_service_method_spec {
    const char* name;

    const struct dbus_service_argument_spec* arguments;
    uint32_t argument_count;
};

struct dbus_service_interface_spec {
    const char* name;

    const struct dbus_service_method_spec* methods;
    uint32_t method_count;
};

struct dbus_service_spec {
    const struct dbus_service_interface_spec* interfaces;
    uint32_t interface_count;

    void* user_data;
};

typedef struct dbus_service dbus_service_t;

dbus_service_t* dbus_service_register(DBusConnection* connection,
                                      const struct dbus_service_spec* spec);

void dbus_service_unregister(dbus_service_t* service);

const char* dbus_service_introspect(dbus_service_t* service);

#endif
