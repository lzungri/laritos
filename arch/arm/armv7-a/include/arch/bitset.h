#pragma once

#include <stdint.h>

typedef uint32_t bitset_t;

static inline uint8_t arch_bitset_ffz(bitset_t bs) {
    uint8_t ret;
    // ARM doesn't have ffz, so we need to implement it via
    // clz (count leading zeros)
    asm volatile (
        // Since we are using clz, we need to work on the negated value
        "mvn %1, %1 \n"
        "clz %0, %1 \n"
        : "=r" (ret)
        : "r" (bs));
    return ret == 32 ? 0xFF : ret;
}

static inline void arch_bitset_lm_set(bitset_t *bs, uint8_t pos) {
    asm volatile (
        "orr %0, %0, %1 \n"
        // +r: bs is used as both input and output
        : "+r" (*bs)
        : "r" (1 << (32 - pos - 1)));
}

static inline void arch_bitset_lm_clear(bitset_t *bs, uint8_t pos) {
    asm volatile (
        "bic %0, %0, %1 \n"
        // +r: bs is used as both input and output
        : "+r" (*bs)
        : "r" (1 << (32 - pos - 1)));
}

static inline uint8_t arch_bitset_lm_bit(bitset_t bs, uint8_t pos) {
    uint32_t ret;
    asm volatile (
        "and %0, %1, %2 \n"
        : "=r" (ret)
        : "r" (bs), "r" (1 << (32 - pos - 1)));
    return ret > 0 ? 1 : 0;
}
