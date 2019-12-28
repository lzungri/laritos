#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <sync/semaphore.h>
#include <core.h>
#include <process/core.h>
#include <sched/core.h>
#include <generated/autoconf.h>

int sem_init(sem_t *sem, uint16_t count) {
    INIT_LIST_HEAD(&sem->blocked);
    spinlock_init(&sem->lock);
    sem->count = count;
    return 0;
}

int sem_acquire(sem_t *sem) {
    verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u)", sem, sem->count, process_get_current()->pid);

    irqctx_t ctx;
    spinlock_acquire(&sem->lock, &ctx);

    while (sem->count == 0) {
        // TODO NON process mode

        pcb_t *pcb = process_get_current();
        list_add_tail(&pcb->sched.blockedlst, &sem->blocked);

        irqctx_t ctx2;
        spinlock_acquire(&_laritos.proclock, &ctx2);
        sched_move_to_blocked_locked(pcb);
        spinlock_release(&_laritos.proclock, &ctx2);

        verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u) -> NOT AVAILABLE", sem, sem->count, process_get_current()->pid);
        spinlock_release(&sem->lock, &ctx);

        schedule();

        spinlock_acquire(&sem->lock, &ctx);
    }
    sem->count--;

    verbose_async("sem_acquire(sem=0x%p, count=%u, pid=%u) -> ACQUIRED", sem, sem->count, process_get_current()->pid);
    spinlock_release(&sem->lock, &ctx);
    return 0;
}

int sem_release(sem_t *sem) {
    irqctx_t ctx;
    spinlock_acquire(&sem->lock, &ctx);

    bool proc_awakened = false;
    if (sem->count == 0) {
        pcb_t *pcb = list_first_entry_or_null(&sem->blocked, pcb_t, sched.blockedlst);
        if (pcb != NULL) {
            list_del_init(&pcb->sched.blockedlst);

            verbose_async("sem_release(sem=0x%p, count=%u, pid=%u) -> waking up pid=%u", sem, sem->count, process_get_current()->pid, pcb->pid);
            irqctx_t ctx2;
            spinlock_acquire(&_laritos.proclock, &ctx2);
            sched_move_to_ready_locked(pcb);
            spinlock_release(&_laritos.proclock, &ctx2);

            proc_awakened = true;
        }
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
