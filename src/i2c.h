#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>

typedef struct i2c_bus i2c_bus_t;
typedef struct i2c_device i2c_device_t;

typedef enum i2c_address_type { I2C_ADDRESS_7_BITS = 0, I2C_ADDRESS_10_BITS } i2c_address_type;

struct i2c_bus_config {
    i2c_address_type addr_type;
    uint32_t delay_ms;
};

i2c_bus_t* i2c_bus_open(uint32_t index, const struct i2c_bus_config* config);
void i2c_bus_close(i2c_bus_t* bus);

void i2c_bus_set_config(i2c_bus_t* bus, const struct i2c_bus_config* config);

i2c_device_t* i2c_device_open(i2c_bus_t* bus, uint16_t address);
void i2c_device_close(i2c_device_t* device);

ssize_t i2c_device_read(i2c_device_t* device, uint8_t internal_addr, void* buffer, size_t length);
ssize_t i2c_device_write(i2c_device_t* device, uint8_t internal_addr, const void* buffer,
                         size_t length);

#endif
