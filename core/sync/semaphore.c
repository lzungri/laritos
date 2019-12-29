#define DEBUG
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

    SLEEP_UNTIL(sem->count > 0, &sem->cond, &sem->lock, &ctx);
    sem->count--;

    verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u) -> ACQUIRED", sem, sem->count, process_get_current()->pid);
    spinlock_release(&sem->lock, &ctx);
    return 0;
}

int sem_release(sem_t *sem) {
    irqctx_t ctx;
    spinlock_acquire(&sem->lock, &ctx);

    bool proc_awakened = false;
    if (condition_notify(&sem->cond) != NULL) {
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
