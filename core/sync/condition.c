#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <process/core.h>
#include <utils/assert.h>
#include <dstruct/list.h>
#include <sync/condition.h>
#include <sched/core.h>

int condition_init(condition_t *cond) {
    INIT_LIST_HEAD(&cond->blocked);
    return 0;
}

static void condition_block_current_process(condition_t *cond, bool proclocked) {
    pcb_t *pcb = process_get_current();

    assert(list_empty(&pcb->sched.blockedlst), "A process can only be waiting for max 1 event");
    list_add_tail(&pcb->sched.blockedlst, &cond->blocked);

    if (proclocked) {
        sched_move_to_blocked_locked(pcb);
    } else {
        irqctx_t ctx2;
        spinlock_acquire(&_laritos.proclock, &ctx2);
        sched_move_to_blocked_locked(pcb);
        spinlock_release(&_laritos.proclock, &ctx2);
    }

    verbose_async("pid=%u waiting for condition=0x%p", pcb->pid, cond);
}

void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx) {
    condition_block_current_process(cond, false);

    spinlock_release(spin, ctx);
    schedule();
    spinlock_acquire(spin, ctx);
}

void condition_wait_proclocked(condition_t *cond, irqctx_t *ctx) {
    condition_block_current_process(cond, true);

    spinlock_release(&_laritos.proclock, ctx);
    schedule();
    spinlock_acquire(&_laritos.proclock, ctx);
}

static inline void wakeup_pcb(pcb_t *pcb, condition_t *cond, bool proclocked) {
    if (pcb == NULL) {
        return;
    }

    list_del_init(&pcb->sched.blockedlst);

    verbose_async("Waking up pid=%u waiting for condition=0x%p", pcb->pid, cond);

    if (proclocked) {
        sched_move_to_ready_locked(pcb);
    } else {
        irqctx_t ctx2;
        spinlock_acquire(&_laritos.proclock, &ctx2);
        sched_move_to_ready_locked(pcb);
        spinlock_release(&_laritos.proclock, &ctx2);
    }
}

static inline pcb_t *notify_locked(condition_t *cond, bool proclocked) {
    pcb_t *pcb = list_first_entry_or_null(&cond->blocked, pcb_t, sched.blockedlst);
    wakeup_pcb(pcb, cond, proclocked);
    return pcb;
}

pcb_t *condition_notify_proclocked(condition_t *cond) {
    return notify_locked(cond, true);
}

pcb_t *condition_notify_locked(condition_t *cond) {
    return notify_locked(cond, false);
}

bool notify_all_locked(condition_t *cond, bool proclocked) {
    pcb_t *pcb;
    pcb_t *tmp;
    bool proc_awakened = false;
    list_for_each_entry_safe(pcb, tmp, &cond->blocked, sched.blockedlst) {
        wakeup_pcb(pcb, cond, proclocked);
        proc_awakened = true;
    }
    return proc_awakened;
}

bool condition_notify_all_proclocked(condition_t *cond) {
    return notify_all_locked(cond, true);
}

bool condition_notify_all_locked(condition_t *cond) {
    return notify_all_locked(cond, false);
}
