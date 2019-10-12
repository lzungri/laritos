#pragma once

#include <stdint.h>

typedef uint8_t bitset_t;

static inline uint8_t arch_bitset_ffz(bitset_t bs) {
    int i;
    for (i = 0; i < 8; i++) {
        if ((bs & 0x80) == 0) {
            return i;
        }
        bs <<= 1;
    }
    return 0xFF;
}

static inline void arch_bitset_lm_set(bitset_t *bs, uint8_t pos) {
    *bs = *bs | (1 << (8 - pos - 1));
}

static inline void arch_bitset_lm_clear(bitset_t *bs, uint8_t pos) {
    *bs = *bs & ~(1 << (8 - pos - 1));
}

static inline uint8_t arch_bitset_lm_bit(bitset_t bs, uint8_t pos) {
    return (bs & (1 << (8 - pos - 1))) > 0 ? 1 : 0;
}
