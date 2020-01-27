#pragma once

#include <stdint.h>

/**
 * Initializes internal state of random generator
 */
void random_seed(uint32_t seed);

/**
 * Returns random integer in range [min, max], including both end points
 *
 * @param min: Min value
 * @param max: Max value
 * @return Value in the [min, max] range
 */
int32_t random_int32(int32_t min, int32_t max);
