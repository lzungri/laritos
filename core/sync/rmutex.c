#include <log.h>

#include <stdbool.h>
#include <sync/spinlock.h>
#include <sync/condition.h>
#include <sync/rmutex.h>
#include <core.h>
#include <process/core.h>
#include <sched/core.h>
#include <utils/assert.h>

int rmutex_init(rmutex_t *mutex) {
    mutex->lock_count = 0;
    mutex->owner = NULL;
    spinlock_init(&mutex->lock);
    condition_init(&mutex->cond);
    return 0;
}

int rmutex_acquire(rmutex_t *mutex) {
    pcb_t *cur = process_get_current();
    irqctx_t ctx;
    spinlock_acquire(&mutex->lock, &ctx);

    BLOCK_UNTIL(mutex->owner == cur || mutex->lock_count == 0, &mutex->cond, &mutex->lock, &ctx);
    mutex->lock_count++;
    mutex->owner = cur;

    verbose_async("rmutex_acquire(m=0x%p, count=%u, pid=%u)%s",
            mutex, mutex->lock_count, cur->pid, mutex->lock_count == 1 ? " -> ACQUIRED" : "");
    spinlock_release(&mutex->lock, &ctx);
    return 0;
}

int rmutex_release(rmutex_t *mutex) {
    pcb_t *cur = process_get_current();

    irqctx_t ctx;
    spinlock_acquire(&mutex->lock, &ctx);

    if (mutex->owner != cur) {
        spinlock_release(&mutex->lock, &ctx);
        return -1;
    }

    bool proc_awakened = false;

    mutex->lock_count--;
    if (mutex->lock_count == 0) {
        mutex->owner = NULL;

        if (condition_notify_all_locked(&mutex->cond)) {
            proc_awakened = true;
        }
        verbose_async("rmutex_release(m=0x%p, count=%u, pid=%u) -> RELEASED", mutex, mutex->lock_count, cur->pid);
    } else {
        verbose_async("rmutex_release(m=0x%p, count=%u, pid=%u)", mutex, mutex->lock_count, cur->pid);
    }

    spinlock_release(&mutex->lock, &ctx);

    // Switch to higher priority processes (if any)
    if (proc_awakened) {
        schedule();
    }

    return 0;
}



#ifdef CONFIG_TEST_CORE_SYNC_RMUTEX
#include __FILE__
#endif
