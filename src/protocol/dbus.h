#ifndef UTIL_DBUS_H
#define UTIL_DBUS_H

#include <stdint.h>

// from dbus/dbus.h
typedef struct _GDBusConnection GDBusConnection;

typedef enum dbus_service_argument_direction {
    DBUS_SERVICE_ARGUMENT_IN,
    DBUS_SERVICE_ARGUMENT_OUT,
} dbus_service_argument_direction;

typedef enum dbus_service_argument_type {
    DBUS_SERVICE_ARGUMENT_BYTE,
    DBUS_SERVICE_ARGUMENT_BOOL,
    DBUS_SERVICE_ARGUMENT_INT16,
    DBUS_SERVICE_ARGUMENT_UINT16,
    DBUS_SERVICE_ARGUMENT_INT32,
    DBUS_SERVICE_ARGUMENT_UINT32,
    DBUS_SERVICE_ARGUMENT_INT64,
    DBUS_SERVICE_ARGUMENT_UINT64,
    DBUS_SERVICE_ARGUMENT_DOUBLE,
    DBUS_SERVICE_ARGUMENT_STRING,
    DBUS_SERVICE_ARGUMENT_OBJECT_PATH,
    DBUS_SERVICE_ARGUMENT_TYPE_SIGNATURE,
    DBUS_SERVICE_ARGUMENT_FILE_DESCRIPTOR,
} dbus_service_argument_type;

typedef void (* dbus_service_method_callback_t)();

struct dbus_service_argument_spec {
    const char* name;
    dbus_service_argument_type type;
    dbus_service_argument_direction direction;
};

struct dbus_service_function_signature {
    const char* name;

    const struct dbus_service_argument_spec* arguments;
    uint32_t argument_count;
};

struct dbus_service_method_spec {
    struct dbus_service_function_signature signature;

    dbus_service_method_callback_t callback;
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

int dbus_loop_ref();
void dbus_loop_unref();

dbus_service_t* dbus_service_register(GDBusConnection* connection,
                                      const struct dbus_service_spec* spec);

void dbus_service_unregister(dbus_service_t* service);

const char* dbus_service_introspect(dbus_service_t* service);

#endif
