#include "protocol/bluetooth.h"

#include "core/map.h"

#include <stdint.h>

#include <malloc.h>

#include <dbus/dbus.h>
#include <gio/gio.h>

#include <pthread.h>

struct bluetooth_main_loop {
    GMainLoop* glib_loop;

    pthread_t loop_thread;
    int thread_started;

    int32_t references;
};

struct bluetooth {
    DBusConnection* connection;

    // maps string (path) to agent ptr
    map_t* agents;
};

struct bluetooth_agent {
    bluetooth_t* connection;
};

struct bluetooth_main_loop* bluetooth_loop = NULL;

void* bluetooth_loop_run(void* arg) {
    struct bluetooth_main_loop* loop;

    loop = (struct bluetooth_main_loop*)arg;
    g_main_loop_run(loop->glib_loop);

    return NULL;
}

void bluetooth_loop_unref() {
    if (!bluetooth_loop) {
        return;
    }

    bluetooth_loop->references--;
    if (bluetooth_loop->references > 0) {
        return;
    }

    if (bluetooth_loop->glib_loop) {
        g_main_loop_quit(bluetooth_loop->glib_loop);

        if (bluetooth_loop->thread_started) {
            pthread_join(bluetooth_loop->loop_thread, NULL);
        }

        g_main_loop_unref(bluetooth_loop->glib_loop);
    }

    free(bluetooth_loop);
    bluetooth_loop = NULL;
}

int bluetooth_loop_ref() {
    if (!bluetooth_loop) {
        bluetooth_loop = (struct bluetooth_main_loop*)malloc(sizeof(struct bluetooth_main_loop));
        bluetooth_loop->references = 0;
        bluetooth_loop->thread_started = 0;

        bluetooth_loop->glib_loop = g_main_loop_new(NULL, TRUE);
        if (!bluetooth_loop->glib_loop) {
            bluetooth_loop_unref();
            return 0;
        }

        if (pthread_create(&bluetooth_loop->loop_thread, NULL, bluetooth_loop_run,
                           bluetooth_loop)) {
            bluetooth_loop_unref();
            return 0;
        }

        bluetooth_loop->thread_started = 1;
    }

    bluetooth_loop->references++;
    return 1;
}

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
