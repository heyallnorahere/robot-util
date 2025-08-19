#include "protocol/dbus.h"

#include <malloc.h>

#include <gio/gio.h>

#include <pthread.h>

struct dbus_main_loop {
    GMainLoop* glib_loop;

    pthread_t loop_thread;
    int thread_started;

    int32_t references;
};

struct dbus_service {
    GDBusConnection* connection;

    char* introspection;
};

struct dbus_main_loop* dbus_loop = NULL;

void* dbus_loop_run(void* arg) {
    struct dbus_main_loop* loop;

    loop = (struct dbus_main_loop*)arg;
    g_main_loop_run(loop->glib_loop);

    return NULL;
}

int dbus_loop_ref() {
    if (!dbus_loop) {
        dbus_loop = (struct dbus_main_loop*)malloc(sizeof(struct dbus_main_loop));
        dbus_loop->references = 0;
        dbus_loop->thread_started = 0;

        dbus_loop->glib_loop = g_main_loop_new(NULL, TRUE);
        if (!dbus_loop->glib_loop) {
            dbus_loop_unref();
            return 0;
        }

        if (pthread_create(&dbus_loop->loop_thread, NULL, dbus_loop_run, dbus_loop)) {
            dbus_loop_unref();
            return 0;
        }

        dbus_loop->thread_started = 1;
    }

    dbus_loop->references++;
    return 1;
}

void dbus_loop_unref() {
    if (!dbus_loop) {
        return;
    }

    dbus_loop->references--;
    if (dbus_loop->references > 0) {
        return;
    }

    if (dbus_loop->glib_loop) {
        g_main_loop_quit(dbus_loop->glib_loop);

        if (dbus_loop->thread_started) {
            pthread_join(dbus_loop->loop_thread, NULL);
        }

        g_main_loop_unref(dbus_loop->glib_loop);
    }

    free(dbus_loop);
    dbus_loop = NULL;
}

dbus_service_t* dbus_service_register(GDBusConnection* connection,
                                      const struct dbus_service_spec* spec) {
    dbus_service_t* service;

    if (!dbus_loop_ref()) {
        return NULL;
    }

    service = (dbus_service_t*)malloc(sizeof(dbus_service_t));

    service->connection = connection;
    g_object_ref(connection);

    // todo: generate introspection XML
    service->introspection = NULL;

    // todo: register

    return service;
}

void dbus_service_unregister(dbus_service_t* service) {
    if (!service) {
        return;
    }

    // todo: unregister service

    g_object_unref(service->connection);

    free(service->introspection);
    free(service);
}

const char* dbus_service_introspect(dbus_service_t* service) { return service->introspection; }
