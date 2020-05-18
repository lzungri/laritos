#pragma once

#include <log.h>
#include <stdint.h>
#include <math.h>
#include <utils/utils.h>

/**
 * Q11.5:
 *   range: [-(2**10); 2**10 - 2**-5] = [-1024; 1023.96875]
 *   resolution: 2**-5 = 0.03125
 */
typedef int16_t q11_5_t;

extern const uint32_t _q11_5_fractions[];

static inline uint8_t q11_5_as_fraction(uint32_t v) {
    int i;
    for (i = 0; i < 32; i++) {
        if (v <= _q11_5_fractions[i]) {
            return i;
        }
    }
    return 31;
}

#define Q11_5(_int, _frac) \
    (q11_5_t) (((abs(_int)) << 5 | q11_5_as_fraction(_frac)) * ((_int) < 0 ? -1 : 1))

#define Q11_5_INT(_v) (int16_t) ((_v) >> 5)
#define Q11_5_FRAC(_v) (uint32_t) (_q11_5_fractions[(_v) & 0x001F])

static inline q11_5_t _saturate16(int32_t v) {
    if (v > 0x7FFF) {
        return 0x7FFF;
    }
    if (v < -0x8000) {
        return -0x8000;
    }
    return (int16_t) v;
}

static inline q11_5_t q11_5_add(q11_5_t v1, q11_5_t v2) {
    return _saturate16((uint32_t) v1 + (uint32_t) v2);
}

static inline q11_5_t q11_5_sub(q11_5_t v1, q11_5_t v2) {
    return _saturate16((uint32_t) v1 - (uint32_t) v2);
}

static inline q11_5_t q11_5_mul(q11_5_t v1, q11_5_t v2) {
    int32_t mul = (int32_t) v1 * (int32_t) v2;
    // Round up
    mul += 1 << (5 - 1);
    return _saturate16(mul >> 5);
}
