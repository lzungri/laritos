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

void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx) {
    pcb_t *pcb = process_get_current();
    sched_move_to_blocked_locked(pcb, &cond->blocked);
    verbose_async("pid=%u waiting for condition=0x%p", pcb->pid, cond);

    spinlock_release(spin, ctx);
    schedule();
    spinlock_acquire(spin, ctx);
}

static inline void wakeup_pcb_locked(pcb_t *pcb, condition_t *cond) {
    if (pcb == NULL) {
        return;
    }
    verbose_async("Waking up pid=%u waiting for condition=0x%p", pcb->pid, cond);
    list_del_init(&pcb->sched.sched_node);
    sched_move_to_ready_locked(pcb);
}

pcb_t *condition_notify_locked(condition_t *cond) {
    pcb_t *pcb = list_first_entry_or_null(&cond->blocked, pcb_t, sched.sched_node);
    wakeup_pcb_locked(pcb, cond);
    return pcb;
}

bool condition_notify_all_locked(condition_t *cond) {
    pcb_t *pcb;
    pcb_t *tmp;
    bool proc_awakened = false;
    list_for_each_entry_safe(pcb, tmp, &cond->blocked, sched.sched_node) {
        wakeup_pcb_locked(pcb, cond);
        proc_awakened = true;
    }
    return proc_awakened;
}
