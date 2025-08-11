#include "i2c.h"

#include <malloc.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "util.h"

#define DEVICE_PATH_LENGTH 19

struct i2c_bus {
    int fd;
    uint32_t index;

    struct i2c_bus_config config;
};

struct i2c_device {
    i2c_bus_t* bus;
    uint16_t address;
};

i2c_bus_t* i2c_bus_open(uint32_t index, const struct i2c_bus_config* config) {
    i2c_bus_t* bus;
    char filename[DEVICE_PATH_LENGTH + 1];

    bus = (i2c_bus_t*)malloc(sizeof(i2c_bus_t));
    bus->index = index;

    snprintf(filename, DEVICE_PATH_LENGTH, "/dev/i2c-%u", index);
    bus->fd = open(filename, O_RDWR);

    if (bus->fd < 0) {
        perror("open");

        i2c_bus_close(bus);
        return NULL;
    }

    i2c_bus_set_config(bus, config);
    return bus;
}

void i2c_bus_close(i2c_bus_t* bus) {
    if (bus->fd >= 0) {
        close(bus->fd);
    }

    free(bus);
}

void i2c_bus_set_config(i2c_bus_t* bus, const struct i2c_bus_config* config) {
    if (config) {
        memcpy(&bus->config, config, sizeof(struct i2c_bus_config));
        return;
    }

    // if no config was passed, set default
    bus->config.addr_type = I2C_ADDRESS_7_BITS;
}

int i2c_bus_select(i2c_bus_t* bus, uint16_t address) {
    int error;

    error = ioctl(bus->fd, I2C_TENBIT, bus->config.addr_type == I2C_ADDRESS_10_BITS);
    if (error) {
        perror("ioctl I2C_TENBIT");

        return 0;
    }

    // gross terminology. use "device address" or something
    error = ioctl(bus->fd, I2C_SLAVE, (unsigned long)address);
    if (error) {
        perror("ioctl I2C_SLAVE");

        return 0;
    }

    return 1;
}

i2c_device_t* i2c_device_open(i2c_bus_t* bus, uint16_t address) {
    i2c_device_t* device;

    device = (i2c_device_t*)malloc(sizeof(i2c_device_t));
    device->bus = bus;
    device->address = address;

    return device;
}

void i2c_device_close(i2c_device_t* device) {
    // we dont need to free anything else
    free(device);
}

ssize_t i2c_device_read(i2c_device_t* device, void* buffer, size_t length) {
    int fd;
    size_t remaining, offset;
    ssize_t bytes_read;
    void* offset_buffer;

    if (!i2c_bus_select(device->bus, device->address)) {
        return -1;
    }

    fd = device->bus->fd;

    remaining = length;
    offset = 0;

    do {
        offset_buffer = buffer + offset;
        bytes_read = read(fd, buffer, remaining);

        if (bytes_read < 0) {
            perror("i2c read");
            return -1;
        }

        if (bytes_read == 0) {
            break;
        }

        remaining -= bytes_read;
        offset += bytes_read;
    } while (remaining > 0);

    return offset;
}

ssize_t i2c_device_write(i2c_device_t* device, const void* buffer, size_t length) {
    int fd;
    size_t remaining, offset;
    ssize_t bytes_written;
    const void* offset_buffer;

    if (!i2c_bus_select(device->bus, device->address)) {
        return -1;
    }

    fd = device->bus->fd;

    remaining = length;
    offset = 0;

    do {
        offset_buffer = buffer + offset;
        bytes_written = write(fd, offset_buffer, remaining);

        if (bytes_written < 0) {
            perror("i2c write");
            return -1;
        }

        if (bytes_written == 0) {
            break;
        }

        remaining -= bytes_written;
        offset += bytes_written;
    } while (remaining > 0);

    return offset;
}
