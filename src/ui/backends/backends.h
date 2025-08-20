#ifndef BACKENDS_H
#define BACKENDS_H

#include <stdint.h>

// from ui/app.h
typedef struct app_backend app_backend_t;

struct robot_util_config;

app_backend_t* app_backend_embedded(const struct robot_util_config* config);

app_backend_t* app_backend_curses();

#endif
