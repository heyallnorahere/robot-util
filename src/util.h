#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#define ARRAYSIZE(ARR) ((size_t)(sizeof(ARR) / sizeof(*(ARR))))

char* util_copy_string(const char* src);

void util_sleep_ms(uint32_t ms);

#endif
