#pragma once

#include <log.h>
#include <stdint.h>
#include <stdbool.h>
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
    /**
     * v1 = int1 * 32 | frac1
     * v2 = int2 * 32 | frac2
     *
     * v1 * v2 = (((int1 * int2) * 32 * 32) | (frac1 * frac2))
     *         = q11_10_res = q11_5_res * 32
     * q11_5_res = (v1 * v2) >> 5
     */
    int32_t mul = (int32_t) v1 * (int32_t) v2;
    // Round up by adding half of the smallest fractional value (2**-5 / 2)
    mul += 1 << (5 - 1);
    // Rescale back to q11.5 with saturation
    return _saturate16(mul >> 5);
}

static inline q11_5_t q11_5_div(q11_5_t v1, q11_5_t v2) {
    /**
     * v1 = int1 * 32 | frac1
     * v2 = int2 * 32 | frac2
     *
     * v1 / v2 = ((int1 * 32) / (int2 * 32)) | (frac1 / frac2) =
     *         = (int1 / int2) | (frac1 / frac2) =
     *         = q16_0_res
     *
     * v1 * 32 / v2 = ((int1 * 32 * 32) / (int2 * 32)) | (frac1 / frac2) =
     *              = ((int1 * 32) / int2) | (frac1 / frac2) =
     *              = ((int1 / int2) * 32) | (frac1 / frac2) =
     *              = q11_5_res
     */
    int32_t scaled = ((int32_t) v1) << 5;
    return _saturate16(scaled / v2);
}
