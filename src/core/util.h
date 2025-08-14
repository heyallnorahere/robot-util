#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>

#define ARRAYSIZE(ARR) ((size_t)(sizeof(ARR) / sizeof(*(ARR))))

void util_sleep_us(uint32_t us);
void util_sleep_ms(uint32_t ms);

void util_set_bit_flag(uint8_t* dst, uint8_t flag, int enabled);

void* util_read_file(const char* path, size_t* size);
ssize_t util_write_file(const char* path, const void* data, size_t size);

char* util_get_dirname(const char* path);

// exact same calling convention as mkdir
// https://man7.org/linux/man-pages/man2/mkdir.2.html
int util_mkdir_recursive(const char* pathname, mode_t mode);

#endif
