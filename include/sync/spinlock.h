#pragma once

#include <stdint.h>
#include <irq.h>
#include <sync/cmpxchg.h>
#include <arch/spinlock.h>

static inline int spinlock_init(spinlock_t *lock) {
    arch_spinlock_set(lock, 0);
    return 0;
}

static inline int spinlock_acquire(spinlock_t *lock, irqctx_t *ctx) {
    // Only disable irq locally. If the irq is handled by another processor,
    // it doesn't block the cur processor holding the lock, thus it can
    // eventually release it
    if (disable_local_irq_save_ctx(ctx) < 0) {
        return -1;
    }
    if (arch_spinlock_acquire(lock) < 0) {
        local_irq_restore_ctx(ctx);
        return -1;
    }
    return 0;
}

static inline int spinlock_release(spinlock_t *lock, irqctx_t *ctx) {
    arch_spinlock_release(lock);
    return local_irq_restore_ctx(ctx);
}

