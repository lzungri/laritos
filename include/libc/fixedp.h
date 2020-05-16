#pragma once

#include <log.h>
#include <stdint.h>

/**
 * Q11.5:
 *   range: [-(2**10); 2**10 - 2**-5] = [-1024; 1023.96875]
 *   resolution: 2**-5 = 0.03125
 */
typedef int16_t q11_5_t;

#define Q11_5(_int, _frac) \
    (q11_5_t) { 0 }

#define Q11_5_INT(_v) (int16_t) ((_v) >> 5)
#define Q11_5_FRAC(_v) (int16_t) ((_v) & 0x001F)

static inline q11_5_t q11_5_add(q11_5_t v1, q11_5_t v2) {
    return 0;
}

static inline q11_5_t q11_5_sub(q11_5_t v1, q11_5_t v2) {
    return 0;
}
