#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * From the ARM ARM document:
 *    In ARMv7, the single-copy atomic processor accesses are:
 *    • All byte accesses.
 *    • All halfword accesses to halfword-aligned locations.
 *    • All word accesses to word-aligned locations.
 *    • Memory accesses caused by a LDREXD/STREXD to a doubleword-aligned
 *      location for which the STREXD succeeds cause single-copy atomic
 *      updates of the doubleword being accessed.
 *
 *    NOTE:
 *      The way to atomically load two 32-bit quantities is to perform a
 *      LDREXD/STREXD sequence, reading and writing the same value, for
 *      which the STREXD succeeds, and use the read values.
 */


/**
 * 32-bit atomic type for ARM.
 * Must be 4-byte aligned to ensure read/write atomicity.
 */
typedef int32_t atomic32_t __attribute__ ((aligned (4)));

/**
 * 64-bit atomic type for ARM.
 * Must be 8-byte aligned to ensure read/write atomicity.
 *
 * TODO: For some reason aligned(8) doesn't seem to be working so I worked around
 * it with aligned(16)
 */
typedef int64_t atomic64_t __attribute__ ((aligned (16)));


#define ATOMIC32_INIT(_value) { _value }
#define ATOMIC64_INIT(_value) { _value }


static inline void arch_atomic32_init(atomic32_t *at, int32_t value) {
    *at = value;
}

static inline int32_t arch_atomic32_get(atomic32_t *at) {
    return *at;
}

static inline int32_t arch_atomic32_add(atomic32_t *at, int32_t v) {
    int32_t prev = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into prev and notify the exclusive monitor */
        "1: ldrex %0, [%2]     \n"
        /* Add v to prev */
        "   add %0, %0, %3     \n"
        /* Store prev into *at */
        "   strex %1, %0, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1         \n"
        /* If not (=1), then keep trying */
        "   beq 1b             \n"
        : "=&r" (prev) /* Declare as output, we want this var to be updated
                          with the last value written to its associated register*/
        : "r" (strex_failed), "r" (at), "r" (v)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return prev;
}

static inline int32_t arch_atomic32_inc(atomic32_t *at) {
    return arch_atomic32_add(at, 1);
}

static inline int32_t arch_atomic32_dec(atomic32_t *at) {
    return arch_atomic32_add(at, -1);
}

static inline int64_t arch_atomic32_set(atomic32_t *at, int64_t value) {
    return *at = value;
}


static inline void arch_atomic64_init(atomic64_t *at, int64_t value) {
    *at = value;
}

static inline int64_t arch_atomic64_get(atomic64_t *at) {
    int64_t prev = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into prev and notify the exclusive monitor */
        "1: ldrexd %0, [%2]     \n"
        /* Store prev into *at */
        "   strexd %1, %0, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1          \n"
        /* If not (=1), then keep trying */
        "   beq 1b              \n"
        : "=&r" (prev) /* Declare as output, we want this var to be updated
                          with the last value written to its associated register*/
        : "r" (strex_failed), "r" (at)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return prev;
}

static inline int64_t arch_atomic64_add(atomic64_t *at, int64_t v) {
    int64_t res = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into res (%0 for the lowest register and %H0 for the highest reg).
         * Also, notify the exclusive monitor */
        "1: ldrexd %0, %H0, [%2]     \n"
        /* Add (S = updating the conditions flags) least significant halves of v
         * and res */
        "   adds %Q0, %Q0, %Q3       \n"
        /* Add (C = add the carry of the previous operation) the most significant
         * halves of v and res */
        "   adc %R0, %R0, %R3        \n"
        /* Store lowest and highest register associated with res into *at */
        "   strexd %1, %0, %H0, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1               \n"
        /* If not (=1), then keep trying */
        "   beq 1b                   \n"
        : "=&r" (res)
        : "r" (strex_failed), "r" (at), "r" (v)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return res;
}

static inline int64_t arch_atomic64_inc(atomic64_t *at) {
    return arch_atomic64_add(at, 1);
}

static inline int64_t arch_atomic64_dec(atomic64_t *at) {
    return arch_atomic64_add(at, -1);
}

static inline int64_t arch_atomic64_set(atomic64_t *at, int64_t value) {
    int64_t prev = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into prev and notify the exclusive monitor */
        "1: ldrexd %0, [%2]     \n"
        /* Store value into *at */
        "   strexd %1, %3, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1          \n"
        /* If not (=1), then keep trying */
        "   beq 1b              \n"
        : "=&r" (prev) /* Declare as output, we want this var to be updated
                          with the last value written to its associated register*/
        : "r" (strex_failed), "r" (at), "r" (value)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return value;
}
