#include "ui/menus/menus.h"

#include "ui/menu.h"
#include "ui/app.h"

#include "core/config.h"

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>

struct main_menu {
    struct robot_util_config* config;
    app_t* const* app;
};

app_t* main_menu_get_app(const struct main_menu* data) {
    if (!data->app) {
        return NULL;
    }

    return *data->app;
}

void main_menu_update_robot(void* user_data) {
    static const size_t max_header_length = 255;

    CURL* curl;
    CURLcode result;
    struct curl_slist* headers;

    struct main_menu* data;
    const char* url;
    char* auth_token;

    char header_buffer[max_header_length + 1];
    char error_buffer[CURL_ERROR_SIZE];

    printf("Robot update requested\n");

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize libcurl! Aborting update request\n");
    }

    data = (struct main_menu*)user_data;
    url = data->config->update_url;

    auth_token = getenv("AUTH_TOKEN");
    headers = NULL;

    if (auth_token) {
        memset(header_buffer, 0, (max_header_length + 1) * sizeof(char));
        snprintf(header_buffer, max_header_length, "Authorization: Bearer %s", auth_token);
        headers = curl_slist_append(headers, header_buffer);

        printf("Using authorization in update request\n");
    } else {
        fprintf(stderr, "Not using authorization in update request! This may cause errors\n");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "CURL error: %s\n", error_buffer);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void main_menu_exit(void* user_data) {
    struct main_menu* data;
    app_t* app;

    data = (struct main_menu*)user_data;
    app = main_menu_get_app(data);

    if (app) {
        app_pop_menu(app);
    } else {
        fprintf(stderr, "No app attached to menu!");
    }
}

void free_main_menu(void* user_data) {
    struct main_menu* data;

    data = (struct main_menu*)user_data;
    
    config_destroy(data->config);
    free(data->config);

    free(data);
}

menu_t* menus_main(struct robot_util_config* config, app_t* const* app) {
    struct main_menu* data;
    menu_t* menu;

    data = (struct main_menu*)malloc(sizeof(struct main_menu));
    data->config = config;
    data->app = app;

    menu = menu_create();
    menu_set_user_data(menu, data, free_main_menu);

    if (config->update_url) {
        menu_add(menu, "Update robot", main_menu_update_robot);
    }

    menu_add(menu, "Exit", main_menu_exit);

    return menu;
}
