#include "protocol/bluetooth.h"

#include "core/map.h"
#include "core/list.h"

#include "protocol/dbus.h"

#include <stdint.h>

#include <malloc.h>
#include <string.h>

#include <gio/gio.h>

#include <pthread.h>

#define BLUEZ_BUS_NAME "org.bluez"
#define DEVICE_INTERFACE_NAME "org.bluez.Device1"

struct bluetooth_device {
    GDBusProxy* proxy;
    GVariant* properties;

    char* address;
    char* name;

    char* path;
};

struct bluetooth {
    GDBusConnection* connection;
    GDBusObjectManager* manager;

    // maps string (path) to agent ptr
    map_t* agents;

    // maps string (address) to device ptr
    map_t* devices;
};

struct bluetooth_agent {
    bluetooth_t* connection;
};

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

    free(device->address);
    free(device->name);
    free(device->path);

    g_variant_unref(device->properties);
    g_object_unref(device->proxy);

    free(device);
}

void bluetooth_device_alloc(bluetooth_t* bt, const char* path, GDBusInterfaceInfo* interface_info) {
    bluetooth_device_t* device;

    GVariant* variant;
    const gchar* string;

    GError* error;

    GVariant* arguments;
    GVariant* interface_name;

    device = (bluetooth_device_t*)malloc(sizeof(bluetooth_device_t));

    error = NULL;
    device->proxy =
        g_dbus_proxy_new_sync(bt->connection, G_DBUS_PROXY_FLAGS_NONE, interface_info,
                              BLUEZ_BUS_NAME, path, DEVICE_INTERFACE_NAME, NULL, &error);

    if (!device->proxy) {
        fprintf(stderr, "Failed to create proxy for device: %s\n", error->message);

        free(device);
        return;
    }

    device->path = strdup(path);

    interface_name = g_variant_new_string(DEVICE_INTERFACE_NAME);
    arguments = g_variant_new_tuple(&interface_name, 1);

    error = NULL;
    g_dbus_connection_call_sync(bt->connection, BLUEZ_BUS_NAME, path,
                                "org.freedesktop.DBus.Properties", "GetAll", NULL, NULL, 0, 0, NULL,
                                &error);

    bluetooth_device_free(bt, device->path);
    map_insert(bt->devices, device->path, device);
}

void bluetooth_interface_added(GDBusObjectManager* manager, GDBusObject* object,
                               GDBusInterface* interface, bluetooth_t* bt) {
    const gchar* object_path;
    GDBusInterfaceInfo* info;

    object_path = g_dbus_object_get_object_path(object);
    info = g_dbus_interface_get_info(interface);

    if (strcmp(info->name, DEVICE_INTERFACE_NAME) == 0) {
        bluetooth_device_alloc(bt, object_path, info);
    }

    // todo: alloc adapter
}

void bluetooth_interface_removed(GDBusObjectManager* manager, GDBusObject* object,
                                 GDBusInterface* interface, bluetooth_t* bt) {
    const gchar* object_path;
    GDBusInterfaceInfo* info;

    object_path = g_dbus_object_get_object_path(object);
    info = g_dbus_interface_get_info(interface);

    if (strcmp(info->name, DEVICE_INTERFACE_NAME) == 0) {
        bluetooth_device_free(bt, object_path);
    }

    // todo: free adapter
}

void bluetooth_object_added(GDBusObjectManager* manager, GDBusObject* object, bluetooth_t* bt) {
    GList* interfaces;
    GList* current_node;

    interfaces = g_dbus_object_get_interfaces(object);
    for (current_node = interfaces; current_node != NULL; current_node = current_node->next) {
        bluetooth_interface_added(manager, object, (GDBusInterface*)current_node->data, bt);
    }

    g_list_free_full(interfaces, g_object_unref);
}

void bluetooth_object_removed(GDBusObjectManager* manager, GDBusObject* object, bluetooth_t* bt) {
    GList* interfaces;
    GList* current_node;

    interfaces = g_dbus_object_get_interfaces(object);
    for (current_node = interfaces; current_node != NULL; current_node = current_node->next) {
        bluetooth_interface_removed(manager, object, (GDBusInterface*)current_node->data, bt);
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
    GError* error = NULL;

    if (!dbus_loop_ref()) {
        return NULL;
    }

    bt = (bluetooth_t*)malloc(sizeof(bluetooth_t));
    bt->agents = map_alloc_string_key(100);
    bt->devices = map_alloc_string_key(100);

    bt->connection = NULL;
    bt->manager = NULL;

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
    list_t* freed_devices;
    list_node_t* current_node;

    if (!bt) {
        return;
    }

    if (bt->manager) {
        g_object_unref(bt->manager);
    }

    if (bt->connection) {
        g_object_unref(bt->connection);
    }

    map_iterate(bt->agents, bluetooth_iterate_free_agent, bt);
    map_free(bt->agents);

    freed_devices = list_alloc();
    map_iterate(bt->devices, bluetooth_map_iterate_collect_keys, bt);

    for (current_node = list_begin(freed_devices); current_node != NULL;
         current_node = list_node_next(current_node)) {
        bluetooth_device_free(bt, (const char*)list_node_get(current_node));
    }

    map_free(bt->devices);
    list_free(freed_devices);

    free(bt);

    dbus_loop_unref();
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
