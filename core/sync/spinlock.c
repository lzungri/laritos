/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdbool.h>
#include <sync/spinlock.h>
#include <core.h>
#include <process/core.h>
#include <utils/assert.h>
#include <generated/autoconf.h>


int spinlock_init(spinlock_t *lock) {
#ifdef CONFIG_SMP
    arch_spinlock_set(&lock->lock, 0);
#endif
    lock->owner = NULL;
    return 0;
}

int spinlock_acquire(spinlock_t *lock, irqctx_t *ctx) {
    // Only disable irqs locally, no need to disable on other cpus:
    //   - If the irqs are not disabled locally, that may lead to a situation in which the irq
    //     handler tries to acquire the same spinlock already locked by the current cpu, causing
    //     a deadlock
    //   - If the irqs are kept enabled for other processors, then if there is an irq handler
    //     trying to grab the same lock, it will just spin on that processor until the cpu owning
    //     the lock releases it
    if (irq_disable_local_and_save_ctx(ctx) < 0) {
        return -1;
    }
#ifdef CONFIG_SMP
    if (arch_spinlock_acquire(&lock->lock) < 0) {
        irq_local_restore_ctx(ctx);
        return -1;
    }
#endif
    // TODO Optimize this
    lock->owner = _laritos.process_mode ? process_get_current() : SPINLOCK_KERNEL_OWNER;
    return 0;
}

bool spinlock_trylock(spinlock_t *lock, irqctx_t *ctx) {
#ifdef CONFIG_SMP
    if (irq_disable_local_and_save_ctx(ctx) < 0) {
        return -1;
    }
    if (arch_spinlock_trylock(&lock->lock)) {
        // TODO Optimize this
        lock->owner = _laritos.process_mode ? process_get_current() : SPINLOCK_KERNEL_OWNER;
        return true;
    }
    irq_local_restore_ctx(ctx);
    return false;
#endif
    return spinlock_acquire(lock, ctx) >= 0;
}

int spinlock_release(spinlock_t *lock, irqctx_t *ctx) {
    lock->owner = NULL;
#ifdef CONFIG_SMP
    arch_spinlock_release(&lock->lock);
#endif
    return irq_local_restore_ctx(ctx);
}

bool spinlock_is_locked(spinlock_t *lock) {
#ifdef CONFIG_SMP
    // This is faster than checking for the owner
    return arch_spinlock_is_locked(&lock->lock);
#endif
    return spinlock_owned_by_me(lock);
}

bool spinlock_owned_by_me(spinlock_t *lock) {
    // TODO Optimize this
    if (_laritos.process_mode) {
        return lock->owner == process_get_current();
    }
    return lock->owner == SPINLOCK_KERNEL_OWNER;
}



#ifdef CONFIG_TEST_CORE_SYNC_SPINLOCK
#include __FILE__
#endif
