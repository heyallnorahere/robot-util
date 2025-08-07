#include "util.h"

#include <string.h>
#include <malloc.h>
#include <stddef.h>

char* util_copy_string(const char* src) {
    char* dst;
    size_t len, total_size;

    len = strlen(src);
    total_size = sizeof(char) * (len + 1);

    dst = (char*)malloc(total_size);
    memset(dst, 0, total_size);
    strncpy(dst, src, len);

    return dst;
}
