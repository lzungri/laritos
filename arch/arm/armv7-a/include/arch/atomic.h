#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * 32-bit atomic type for ARM.
 * Must be 4-byte aligned to ensure read/write atomicity.
 */
typedef int32_t atomic32_t __attribute__ ((aligned (4)));


#define ATOMIC32_INIT(_value) { _value }


/**
 * According to ARM ARM document:
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

static inline void arch_atomic32_init(atomic32_t *at, int32_t value) {
    *at = value;
}

static inline int32_t arch_atomic32_get(atomic32_t *at) {
    return *at;
}

static inline int32_t arch_atomic32_inc(atomic32_t *at) {
    int32_t prev = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into prev and notify the exclusive monitor */
        "1: ldrex %0, [%2]     \n"
        /* Increment prev */
        "   add %0, %0, #1     \n"
        /* Store prev into *at */
        "   strex %1, %0, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1         \n"
        /* If not (=1), then keep trying */
        "   beq 1b             \n"
        : "=&r" (prev) /* Declare as output, we want this var to be updated
                          with the last value written to its associated register*/
        : "r" (strex_failed), "r" (at)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return prev;
}

static inline int32_t arch_atomic32_dec(atomic32_t *at) {
    int32_t prev = 0;
    bool strex_failed = true;
    asm volatile (
        /* Load *at into prev and notify the exclusive monitor */
        "1: ldrex %0, [%2]     \n"
        /* Decrement prev */
        "   sub %0, %0, #1     \n"
        /* Store prev into *at */
        "   strex %1, %0, [%2] \n"
        /* Check whether the store operation was done in an exclusive manner */
        "   cmp %1, #1         \n"
        /* If not (=1), then keep trying */
        "   beq 1b             \n"
        : "=&r" (prev) /* Declare as output, we want this var to be updated
                          with the last value written to its associated register*/
        : "r" (strex_failed), "r" (at)
        : "memory" /* Memory barrier (do not reorder read/writes)*/,
          "cc" /* Tell gcc this code modifies the cpsr */);
    return prev;
}

static inline int32_t arch_atomic32_set(atomic32_t *at, int32_t value) {
    return *at = value;
}
