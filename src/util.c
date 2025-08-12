#include "util.h"

#include <unistd.h>

void util_sleep_us(uint32_t us) { usleep(us); }
void util_sleep_ms(uint32_t ms) { util_sleep_us(ms * 1e3); }

void util_set_bit_flag(uint8_t* dst, uint8_t flag, int enabled) {
    if (enabled) {
        *dst |= flag;
    } else {
        *dst &= ~flag;
    }
}
