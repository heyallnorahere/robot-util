#include "protocol/bluetooth.h"

#include "core/map.h"
#include "core/list.h"

#include "core/util.h"

#include "protocol/dbus.h"

#include <stdint.h>

#include <malloc.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <pthread.h>

#define BLUEZ_BUS_NAME "org.bluez"
#define DEVICE_INTERFACE_NAME "org.bluez.Device1"
#define ADAPTER_INTERFACE_NAME "org.bluez.Adapter1"
#define PROPERTIES_INTERFACE_NAME "org.freedesktop.DBus.Properties"

struct bluetooth_device {
    GDBusProxy* device_proxy;
    GDBusProxy* properties_proxy;

    char* path;

    bluetooth_t* connection;
};

typedef struct bluetooth_adapter {
    GDBusProxy* adapter_proxy;
    GDBusProxy* properties_proxy;

    char* path;
    int initially_discovering;

    bluetooth_t* connection;
} bluetooth_adapter_t;

struct bluetooth {
    GDBusConnection* connection;
    GDBusObjectManager* manager;

    // maps string (path) to agent ptr
    map_t* agents;

    // maps string (path) to device ptr
    map_t* devices;

    // maps string (path) to adapter ptr
    map_t* adapters;

    pthread_mutex_t mutex;
};

struct bluetooth_agent {
    bluetooth_t* connection;
};

// returns with a reference
GVariant* bluetooth_get_property(GDBusProxy* proxy, const char* interface_name, const char* name) {
    GVariant* argument_ptrs[2];
    GVariant* arguments;
    GVariant* retval;

    GVariant* value;

    argument_ptrs[0] = g_variant_new_string(interface_name);
    argument_ptrs[1] = g_variant_new_string(name);
    arguments = g_variant_new_tuple(argument_ptrs, ARRAYSIZE(argument_ptrs));

    retval =
        g_dbus_proxy_call_sync(proxy, "Get", arguments, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);

    if (!retval) {
        return NULL;
    }

    // return value is first element
    value = g_variant_get_child_value(retval, 0);

    // transfer ownership & free tuple
    g_variant_ref(value);
    g_variant_unref(retval);

    if (g_variant_is_of_type(value, G_VARIANT_TYPE("v"))) {
        retval = value;
        value = g_variant_get_variant(retval);
        g_variant_unref(retval);
    }

    return value;
}

void bluetooth_device_free(bluetooth_t* bt, const char* path) {
    bluetooth_device_t* device;
    void* key;
    void* value;

    if (!bt || !path) {
        return;
    }

    key = (void*)path;
    if (!map_get(bt->devices, key, &value)) {
        return;
    }

    device = (bluetooth_device_t*)value;
    map_remove(bt->devices, key);

    g_object_unref(device->device_proxy);
    g_object_unref(device->properties_proxy);

    free(device->path);
    free(device);
}

void bluetooth_device_alloc(bluetooth_t* bt, const char* path, GDBusInterfaceInfo* interface_info) {
    bluetooth_device_t* device;

    GError* error;

    device = (bluetooth_device_t*)malloc(sizeof(bluetooth_device_t));
    device->connection = bt;

    error = NULL;
    device->device_proxy =
        g_dbus_proxy_new_sync(bt->connection, G_DBUS_PROXY_FLAGS_NONE, interface_info,
                              BLUEZ_BUS_NAME, path, DEVICE_INTERFACE_NAME, NULL, &error);

    if (!device->device_proxy) {
        fprintf(stderr, "Failed to create proxy for device: %s\n", error->message);

        free(device);
        return;
    }

    error = NULL;
    device->properties_proxy =
        g_dbus_proxy_new_sync(bt->connection, G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_BUS_NAME, path,
                              PROPERTIES_INTERFACE_NAME, NULL, &error);

    if (!device->properties_proxy) {
        fprintf(stderr, "Failed to create proxy for device properties: %s\n", error->message);

        g_object_unref(device->device_proxy);
        free(device);

        return;
    }

    device->path = strdup(path);

    bluetooth_device_free(bt, device->path);
    map_insert(bt->devices, device->path, device);
}

void bluetooth_adapter_free(bluetooth_t* bt, const char* path) {
    bluetooth_adapter_t* adapter;
    void* key;
    void* value;

    GVariant* retval;

    if (!bt || !path) {
        return;
    }

    key = (void*)path;
    if (!map_get(bt->adapters, key, &value)) {
        return;
    }

    adapter = (bluetooth_adapter_t*)value;
    map_remove(bt->adapters, key);

    if (!adapter->initially_discovering) {
        retval = g_dbus_proxy_call_sync(adapter->adapter_proxy, "StopDiscovery", NULL,
                                        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);

        if (retval) {
            g_variant_unref(retval);
        }

        // we dont care if it failed. not our problem anymore
    }

    g_object_unref(adapter->adapter_proxy);
    g_object_unref(adapter->properties_proxy);

    free(adapter->path);
    free(adapter);
}

void bluetooth_adapter_alloc(bluetooth_t* bt, const char* path, GDBusInterfaceInfo* info) {
    bluetooth_adapter_t* adapter;

    GVariant* value;
    GError* error;

    adapter = (bluetooth_adapter_t*)malloc(sizeof(bluetooth_adapter_t));
    adapter->connection = bt;

    error = NULL;
    adapter->adapter_proxy =
        g_dbus_proxy_new_sync(bt->connection, G_DBUS_PROXY_FLAGS_NONE, info, BLUEZ_BUS_NAME, path,
                              ADAPTER_INTERFACE_NAME, NULL, &error);

    if (!adapter->adapter_proxy) {
        fprintf(stderr, "Failed to create proxy for adapter: %s\n", error->message);

        free(adapter);
        return;
    }

    error = NULL;
    adapter->properties_proxy =
        g_dbus_proxy_new_sync(bt->connection, G_DBUS_PROXY_FLAGS_NONE, info, BLUEZ_BUS_NAME, path,
                              PROPERTIES_INTERFACE_NAME, NULL, &error);

    if (!adapter->properties_proxy) {
        fprintf(stderr, "Failed to create proxy for adapter properties: %s\n", error->message);

        g_object_unref(adapter->adapter_proxy);
        free(adapter);

        return;
    }

    value =
        bluetooth_get_property(adapter->properties_proxy, ADAPTER_INTERFACE_NAME, "Discovering");

    if (value) {
        adapter->initially_discovering = (int)g_variant_get_boolean(value);
        g_variant_unref(value);
    } else {
        adapter->initially_discovering = 1; // dont bother
    }

    if (!adapter->initially_discovering) {
        error = NULL;
        value = g_dbus_proxy_call_sync(adapter->adapter_proxy, "StartDiscovery", NULL,
                                       G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

        if (!value) {
            fprintf(stderr, "Failed to begin discovery for adapter %s: %s\n", path, error->message);
            adapter->initially_discovering = 0;
        } else {
            g_variant_unref(value);
        }
    }

    adapter->path = strdup(path);

    bluetooth_adapter_free(bt, path);
    map_insert(bt->adapters, adapter->path, adapter);
}

void bluetooth_interface_added(GDBusObjectManager* manager, GDBusObject* object,
                               GDBusInterface* interface, bluetooth_t* bt) {
    const gchar* object_path;
    const gchar* interface_name;
    GDBusInterfaceInfo* info;

    object_path = g_dbus_object_get_object_path(object);
    interface_name = g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface));
    info = g_dbus_interface_get_info(interface);

    if (!interface_name) {
        return;
    }

    pthread_mutex_lock(&bt->mutex);

    if (strcmp(interface_name, DEVICE_INTERFACE_NAME) == 0) {
        bluetooth_device_alloc(bt, object_path, info);
    }

    if (strcmp(interface_name, ADAPTER_INTERFACE_NAME) == 0) {
        bluetooth_adapter_alloc(bt, object_path, info);
    }

    pthread_mutex_unlock(&bt->mutex);
}

void bluetooth_interface_removed(GDBusObjectManager* manager, GDBusObject* object,
                                 GDBusInterface* interface, bluetooth_t* bt) {
    const gchar* object_path;
    const gchar* interface_name;

    object_path = g_dbus_object_get_object_path(object);
    interface_name = g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface));

    if (!interface_name) {
        return;
    }

    pthread_mutex_lock(&bt->mutex);

    if (strcmp(interface_name, DEVICE_INTERFACE_NAME) == 0) {
        bluetooth_device_free(bt, object_path);
    }

    if (strcmp(interface_name, ADAPTER_INTERFACE_NAME) == 0) {
        bluetooth_adapter_free(bt, object_path);
    }

    pthread_mutex_unlock(&bt->mutex);
}

void bluetooth_object_added(GDBusObjectManager* manager, GDBusObject* object, bluetooth_t* bt) {
    GList* interfaces;
    GList* current_node;

    interfaces = g_dbus_object_get_interfaces(object);
    for (current_node = interfaces; current_node != NULL; current_node = current_node->next) {
        bluetooth_interface_added(manager, object, G_DBUS_INTERFACE(current_node->data), bt);
    }

    g_list_free_full(interfaces, g_object_unref);
}

void bluetooth_object_removed(GDBusObjectManager* manager, GDBusObject* object, bluetooth_t* bt) {
    GList* interfaces;
    GList* current_node;

    interfaces = g_dbus_object_get_interfaces(object);
    for (current_node = interfaces; current_node != NULL; current_node = current_node->next) {
        bluetooth_interface_removed(manager, object, G_DBUS_INTERFACE(current_node->data), bt);
    }

    g_list_free_full(interfaces, g_object_unref);
}

void bluetooth_scan_bus(bluetooth_t* bt) {
    GList* objects;
    GList* current_node;
    GDBusObject* object;

    objects = g_dbus_object_manager_get_objects(bt->manager);
    for (current_node = objects; current_node != NULL; current_node = current_node->next) {
        object = (GDBusObject*)current_node->data;
        bluetooth_object_added(bt->manager, object, bt);
    }

    g_list_free_full(objects, g_object_unref);
}

int bluetooth_create_object_manager(bluetooth_t* bt) {
    GError* error;

    error = NULL;
    bt->manager = g_dbus_object_manager_client_new_sync(
        bt->connection, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, BLUEZ_BUS_NAME, "/", NULL, NULL,
        NULL, NULL, &error);

    if (!bt->manager) {
        fprintf(stderr, "Error creating object manager client for bus org.bluez: %s\n",
                error->message);

        return 0;
    }

    g_signal_connect_data(bt->manager, "interface-added", (GCallback)bluetooth_interface_added, bt,
                          0, 0);

    g_signal_connect_data(bt->manager, "interface-removed", (GCallback)bluetooth_interface_removed,
                          bt, 0, 0);

    g_signal_connect_data(bt->manager, "object-added", (GCallback)bluetooth_object_added, bt, 0, 0);
    g_signal_connect_data(bt->manager, "object-removed", (GCallback)bluetooth_object_removed, bt, 0,
                          0);

    bluetooth_scan_bus(bt);
    return 1;
}

bluetooth_t* bluetooth_connect() {
    bluetooth_t* bt;
    GError* error;

    if (!dbus_loop_ref()) {
        return NULL;
    }

    bt = (bluetooth_t*)malloc(sizeof(bluetooth_t));
    bt->agents = map_alloc_string_key(100);
    bt->devices = map_alloc_string_key(100);
    bt->adapters = map_alloc_string_key(100);

    bt->connection = NULL;
    bt->manager = NULL;

    pthread_mutex_init(&bt->mutex, NULL);

    error = NULL;
    bt->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (!bt->connection) {
        fprintf(stderr, "Error retrieving system bus: %s\n", error->message);

        bluetooth_disconnect(bt);
        return NULL;
    }

    if (!bluetooth_create_object_manager(bt)) {
        bluetooth_disconnect(bt);
        return NULL;
    }

    return bt;
}

void bluetooth_iterate_free_agent(void* key, void* value, void* user_data) {
    bluetooth_agent_free((bluetooth_agent_t*)value);
}

void bluetooth_map_iterate_collect_keys(void* key, void* value, void* user_data) {
    list_t* list;

    list = (list_t*)user_data;
    list_insert(list, NULL, key);
}

void bluetooth_disconnect(bluetooth_t* bt) {
    list_t* freed_paths;
    list_node_t* current_node;

    if (!bt) {
        return;
    }

    pthread_mutex_destroy(&bt->mutex);

    if (bt->manager) {
        g_object_unref(bt->manager);
    }

    if (bt->connection) {
        g_object_unref(bt->connection);
    }

    map_iterate(bt->agents, bluetooth_iterate_free_agent, bt);
    map_free(bt->agents);

    freed_paths = list_alloc();
    map_iterate(bt->devices, bluetooth_map_iterate_collect_keys, bt);

    for (current_node = list_begin(freed_paths); current_node != NULL;
         current_node = list_node_next(current_node)) {
        bluetooth_device_free(bt, (const char*)list_node_get(current_node));
    }

    map_free(bt->devices);
    list_clear(freed_paths);

    map_iterate(bt->adapters, bluetooth_map_iterate_collect_keys, bt);
    for (current_node = list_begin(freed_paths); current_node != NULL;
         current_node = list_node_next(current_node)) {
        bluetooth_adapter_free(bt, (const char*)list_node_get(current_node));
    }

    map_free(bt->adapters);
    list_free(freed_paths);

    free(bt);

    dbus_loop_unref();
}

struct bluetooth_device_iteration {
    uint32_t count;
    list_t* devices;
};

void bluetooth_map_iterate_devices(void* key, void* value, void* user_data) {
    struct bluetooth_device_iteration* iteration;

    iteration = (struct bluetooth_device_iteration*)user_data;
    iteration->count++;

    list_insert(iteration->devices, list_end(iteration->devices), value);
}

bluetooth_device_t** bluetooth_iterate_devices(bluetooth_t* bt, uint32_t* count) {
    struct bluetooth_device_iteration iteration;

    bluetooth_device_t** device_array;
    list_node_t* current_node;
    size_t index;

    iteration.count = 0;
    iteration.devices = list_alloc();

    pthread_mutex_lock(&bt->mutex);
    map_iterate(bt->devices, bluetooth_map_iterate_devices, &iteration);
    pthread_mutex_unlock(&bt->mutex);

    device_array = (bluetooth_device_t**)malloc(iteration.count * sizeof(void*));
    *count = iteration.count;

    index = 0;
    for (current_node = list_begin(iteration.devices); current_node != NULL;
         current_node = list_node_next(current_node)) {
        device_array[index++] = (bluetooth_device_t*)list_node_get(current_node);
    }

    return device_array;
}

char* bluetooth_device_get_name(bluetooth_device_t* device) {
    GVariant* value;
    char* name;

    value = bluetooth_get_property(device->properties_proxy, DEVICE_INTERFACE_NAME, "Name");
    if (value) {
        name = strdup(g_variant_get_string(value, NULL));
        g_variant_unref(value);
    } else {
        name = NULL;
    }

    return name;
}

char* bluetooth_device_get_address(bluetooth_device_t* device) {
    GVariant* value;
    char* address;

    value = bluetooth_get_property(device->properties_proxy, DEVICE_INTERFACE_NAME, "Address");
    if (value) {
        address = strdup(g_variant_get_string(value, NULL));
        g_variant_unref(value);
    } else {
        address = NULL;
    }

    return address;
}

int bluetooth_device_is_paired(bluetooth_device_t* device) {
    GVariant* value;
    int paired;

    value = bluetooth_get_property(device->properties_proxy, DEVICE_INTERFACE_NAME, "Paired");
    if (value) {
        paired = (int)g_variant_get_boolean(value);
        g_variant_unref(value);
    } else {
        paired = 0;
    }

    return paired;
}

bluetooth_agent_t* bluetooth_agent_create(bluetooth_t* connection, const char* path) {
    bluetooth_agent_t* agent;

    agent = (bluetooth_agent_t*)malloc(sizeof(bluetooth_agent_t));
    agent->connection = connection;

    // todo: register dbus service

    return agent;
}

void bluetooth_agent_free(bluetooth_agent_t* agent) {
    if (!agent) {
        return;
    }

    bluetooth_unregister_agent(agent);

    // todo: unregister dbus service

    free(agent);
}

void bluetooth_register_agent(bluetooth_agent_t* agent) {
    // todo: register agent via dbus
}

void bluetooth_unregister_agent(bluetooth_agent_t* agent) {
    // todo: unregister agent via dbus
}
