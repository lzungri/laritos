#pragma once

#include <stdint.h>
#include <arch/bitset.h>

#define BITSET_NBITS (sizeof(bitset_t) << 3)

#define BITSET_IDX_NOT_FOUND (0xFF)
#define BITSET_ARRAY_IDX_NOT_FOUND (0xFFFFFFFF)

static inline uint8_t bitset_ffz(bitset_t bs) {
    return arch_bitset_ffz(bs);
}

static inline uint32_t bitset_array_ffz(bitset_t bs[], size_t n) {
    int i;
    for (i = 0; i < n; i++) {
        uint8_t relidx = bitset_ffz(bs[i]);
        if (relidx != BITSET_IDX_NOT_FOUND) {
            return i * BITSET_NBITS + relidx;
        }
    }
    return BITSET_ARRAY_IDX_NOT_FOUND;
}

static inline int bitset_lm_set(bitset_t *bs, uint8_t pos) {
    if (pos > BITSET_NBITS) {
        return -1;
    }
    arch_bitset_lm_set(bs, pos);
    return 0;
}

static inline void bitset_array_lm_set(bitset_t bs[], size_t n, uint32_t pos) {
    if (pos == BITSET_ARRAY_IDX_NOT_FOUND) {
        return;
    }
    bitset_lm_set(&bs[pos / BITSET_NBITS], pos % BITSET_NBITS);
}

static inline int bitset_lm_clear(bitset_t *bs, uint8_t pos) {
    if (pos > BITSET_NBITS) {
        return -1;
    }
    arch_bitset_lm_clear(bs, pos);
    return 0;
}

static inline void bitset_array_lm_clear(bitset_t bs[], size_t n, uint32_t pos) {
    if (pos == BITSET_ARRAY_IDX_NOT_FOUND) {
        return;
    }
    bitset_lm_clear(&bs[pos / BITSET_NBITS], pos % BITSET_NBITS);
}

static inline uint8_t bitset_lm_bit(bitset_t bs, uint8_t pos) {
    return arch_bitset_lm_bit(bs, pos);
}

static inline uint8_t bitset_array_lm_bit(bitset_t bs[], size_t n, uint32_t pos) {
    return bitset_lm_bit(bs[pos / BITSET_NBITS], pos % BITSET_NBITS);
}
