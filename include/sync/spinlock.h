#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <irq/core.h>
#include <sync/cmpxchg.h>
#include <arch/spinlock.h>
#include <generated/autoconf.h>

/**
 * Note regarding spinlocks:
 *    If the OS is compiled with the CONFIG_SMP option (i.e. multiprocessing support), then
 *    we need to use a lock to guarantee no other processor is running inside the critical section.
 *    If the OS was compiled without the CONFIG_SMP option (i.e. uniprocessor), then
 *    there is no need to use lock variables, we just need to disable local interrupts, which will
 *    prevent local context switches.
 */

static inline int spinlock_init(spinlock_t *lock) {
#ifdef CONFIG_SMP
    arch_spinlock_set(lock, 0);
#endif
    return 0;
}

static inline int spinlock_acquire(spinlock_t *lock, irqctx_t *ctx) {
    // Only disable irq locally. If the irq is handled by another processor,
    // it doesn't block the cur processor holding the lock, thus it can
    // eventually release it
    if (irq_disable_local_and_save_ctx(ctx) < 0) {
        return -1;
    }
#ifdef CONFIG_SMP
    if (arch_spinlock_acquire(lock) < 0) {
        irq_local_restore_ctx(ctx);
        return -1;
    }
#endif
    return 0;
}

static inline int spinlock_trylock(spinlock_t *lock, irqctx_t *ctx) {
    return arch_spinlock_trylock(lock, ctx);
}

static inline int spinlock_release(spinlock_t *lock, irqctx_t *ctx) {
#ifdef CONFIG_SMP
    arch_spinlock_release(lock);
#endif
    return irq_local_restore_ctx(ctx);
}

static inline bool spinlock_is_taken(spinlock_t *lock) {
    return arch_spinlock_is_taken(lock);
}
