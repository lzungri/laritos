#pragma once

#include <stdint.h>
#include <stdbool.h>

static inline bool arch_atomic_cmpxchg(volatile int *buf, int old, int new) {
    uint32_t lockv;
    bool strex_failed = true;

    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        "1: ldrex %[lockv], [%[buf]]                \n"
        "   cmp %[lockv], %[old]                    \n"
        "   bne 2f                                  \n"
        "   strex %[strex_failed], %[new], [%[buf]] \n"
        "   cmp %[strex_failed], #1                 \n"
        "   beq 1b                                  \n"
        "2:"
        : [lockv] "=&r" (lockv), [strex_failed] "=&r" (strex_failed)
        : [buf] "r" (buf), [old] "r" (old), [new] "r" (new)
        : "memory" /* Memory barrier (do not reorder read/writes)*/ , "cc" /* Tell gcc this code modifies the cpsr */);

    return !strex_failed;
}
