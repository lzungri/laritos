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
#include <sync/semaphore.h>
#include <sync/spinlock.h>
#include <sync/condition.h>
#include <core.h>
#include <process/core.h>
#include <sched/core.h>
#include <utils/assert.h>
#include <generated/autoconf.h>

int sem_init(sem_t *sem, uint16_t count) {
    spinlock_init(&sem->lock);
    sem->count = count;
    condition_init(&sem->cond);
    return 0;
}

int sem_acquire(sem_t *sem) {
    verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u)", sem, sem->count, process_get_current()->pid);

    irqctx_t ctx;
    spinlock_acquire(&sem->lock, &ctx);

    BLOCK_UNTIL(sem->count > 0, &sem->cond, &sem->lock, &ctx);
    sem->count--;

    verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u) -> ACQUIRED", sem, sem->count, process_get_current()->pid);
    spinlock_release(&sem->lock, &ctx);
    return 0;
}

int sem_release(sem_t *sem) {
    irqctx_t ctx;
    spinlock_acquire(&sem->lock, &ctx);

    bool proc_awakened = false;
    if (condition_notify_all_locked(&sem->cond)) {
        proc_awakened = true;
    }
    sem->count++;

    verbose_async("sem_release(sem=0x%p, count=%u, pid=%u)", sem, sem->count, process_get_current()->pid);
    spinlock_release(&sem->lock, &ctx);

    // Switch to higher priority processes (if any)
    if (proc_awakened) {
        schedule();
    }

    return 0;
}



#ifdef CONFIG_TEST_CORE_SYNC_SEMAPHORE
#include __FILE__
#endif
