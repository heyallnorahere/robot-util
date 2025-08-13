#include "util.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>

#include <stdio.h>

#include <malloc.h>
#include <string.h>

void util_sleep_us(uint32_t us) { usleep(us); }
void util_sleep_ms(uint32_t ms) { util_sleep_us(ms * 1e3); }

void util_set_bit_flag(uint8_t* dst, uint8_t flag, int enabled) {
    if (enabled) {
        *dst |= flag;
    } else {
        *dst &= ~flag;
    }
}

void* util_read_file(const char* path, size_t* size) {
    int fd;
    off_t begin, end;

    size_t file_size;
    void* buffer;

    size_t bytes_remaining, offset;
    ssize_t bytes_read;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    end = lseek(fd, 0, SEEK_END);
    begin = lseek(fd, 0, SEEK_SET);

    if (end < 0 || begin < 0) {
        perror("lseek");
        return NULL;
    }

    file_size = end - begin;
    buffer = malloc(file_size);

    bytes_remaining = file_size;
    offset = 0;

    do {
        bytes_read = read(fd, buffer + offset, bytes_remaining);
        if (bytes_read < 0) {
            perror("read");

            free(buffer);
            return NULL;
        }

        if (bytes_read == 0) {
            break;
        }

        offset += bytes_read;
        bytes_remaining -= bytes_read;
    } while (bytes_remaining > 0);

    close(fd);

    *size = offset;
    return buffer;
}

ssize_t util_write_file(const char* path, const void* data, size_t size) {
    int fd;

    int status, exists;
    struct stat file_stat;

    size_t bytes_remaining, offset;
    ssize_t bytes_written;

    exists = 1;
    status = stat(path, &file_stat);

    if (status != 0) {
        if (errno != ENOENT) {
            perror("stat");
            return -1;
        } else {
            exists = 0;
        }
    }

    if (exists) {
        fd = open(path, O_WRONLY);
    } else {
        fd = creat(path, S_IRWXU | S_IRWXG);
    }

    if (fd < 0) {
        perror(exists ? "open" : "creat");
        return -1;
    }

    bytes_remaining = size;
    offset = 0;

    do {
        bytes_written = write(fd, data + offset, bytes_remaining);
        if (bytes_written < 0) {
            perror("write");
            return -1;
        }

        if (bytes_written == 0) {
            break;
        }

        offset += bytes_written;
        bytes_remaining -= bytes_written;
    } while (bytes_remaining > 0);

    close(fd);
    return offset;
}

static const char path_separator = '/';

char* util_get_dirname(const char* path) {
    size_t path_length, dir_length;
    const char* cursor;

    size_t buffer_size;
    char* buffer;

    cursor = path + strlen(path);
    while (cursor != path) {
        cursor--;

        if (*cursor == path_separator) {
            break;
        }
    }

    if (cursor == path && *cursor != path_separator) {
        return NULL;
    }

    dir_length = cursor - path + 1;
    buffer = (char*)malloc((dir_length + 1) * sizeof(char));

    strncpy(buffer, path, dir_length);
    buffer[dir_length] = '\0';

    return buffer;
}

int util_mkdir_recursive(const char* pathname, mode_t mode) {
    const char* cursor;
    size_t current_path_len;

    char* buffer;
    size_t buffer_size;

    int status;

    buffer_size = (strlen(pathname) + 1) * sizeof(char);
    buffer = (char*)malloc(buffer_size);
    memset(buffer, 0, buffer_size);

    cursor = strchr(pathname, path_separator);
    do {
        // skip if this is an empty segment
        if (cursor != pathname && *(cursor - 1) == path_separator) {
            cursor++;
            continue;
        }

        // path names get longer with each iteration, so we dont need to worry about the buffer
        // being zeroed
        current_path_len = cursor - pathname;
        strncpy(buffer, pathname, current_path_len);

        if (current_path_len > 0) {
            status = mkdir(buffer, mode);

            if (status != 0 && errno != EEXIST) {
                free(buffer);
                return -1;
            }
        }

        cursor = strchr(cursor + 1, path_separator);
    } while (cursor);

    if (pathname[strlen(pathname) - 1] != path_separator) {
        status = mkdir(pathname, mode);

        if (status != 0 && errno == EEXIST) {
            status = 0;
        }
    } else {
        status = 0;
    }

    free(buffer);
    return status;
}
