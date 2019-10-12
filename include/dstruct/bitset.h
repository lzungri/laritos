#pragma once

#include <stdint.h>
#include <arch/bitset.h>

#define BITSET_NBITS (sizeof(bitset_t) << 3)

static inline int8_t bitset_ffz(bitset_t bs) {
    return arch_bitset_ffz(bs);
}

static inline int bitset_lm_set(bitset_t *bs, uint8_t pos) {
    if (pos > BITSET_NBITS) {
        return -1;
    }
    arch_bitset_lm_set(bs, pos);
    return 0;
}

static inline int bitset_lm_clear(bitset_t *bs, uint8_t pos) {
    if (pos > BITSET_NBITS) {
        return -1;
    }
    arch_bitset_lm_clear(bs, pos);
    return 0;
}

// TODO bitset_ffz_whole_array
