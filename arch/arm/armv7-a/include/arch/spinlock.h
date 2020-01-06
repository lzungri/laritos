#pragma once

#include <sync/barrier.h>
#include <sync/cmpxchg.h>

/**
 * 32-bit spinlock type for ARM.
 * Must be 4-byte aligned to ensure read/write atomicity.
 */
typedef uint32_t spinlock_t __attribute__ ((aligned (4)));


static inline int arch_spinlock_set(spinlock_t *lock, unsigned int value) {
    *lock = value;
    return 0;
}

static inline int arch_spinlock_acquire(spinlock_t *lock) {
    while (!atomic_cmpxchg((int *) lock, 0, 1));
    /**
     * From ARM ARM:
     *   The synchronization primitives follow the memory order model of the
     *   memory type accessed by the instructions.
     *   For this reason:
     *      * Portable software for claiming a spin-lock must include a
     *      Data Memory Barrier (DMB) operation, performed by a DMB instruction,
     *      between claiming the spin-lock and making any access that makes use of the spin-lock.
     */
    dmb();
    return 0;
}

static inline int arch_spinlock_release(spinlock_t *lock) {
    /**
     * From ARM ARM:
     *      * Portable software for releasing a spin-lock must include a DMB
     *      instruction before writing to clear the spin-lock
     */
    dmb();
    return arch_spinlock_set(lock, 0);
}
