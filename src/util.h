#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#define ARRAYSIZE(ARR) ((size_t)(sizeof(ARR) / sizeof(*(ARR))))

void util_sleep_us(uint32_t us);
void util_sleep_ms(uint32_t ms);

void util_set_bit_flag(uint8_t* dst, uint8_t flag, int enabled);

#endif
