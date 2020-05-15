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
#include <core.h>
#include <process/core.h>
#include <assert.h>
#include <dstruct/list.h>
#include <sync/condition.h>
#include <sched/core.h>

int condition_init(condition_t *cond) {
    INIT_LIST_HEAD(&cond->blocked);
    return 0;
}

/**
 * HACK ALERT: A condition can be used in different scenarios, some of them may or may not
 * have the _laritos.proc.pcbs_data_lock lock held. In order to prevent deadlocks by
 * re-acquiring the same lock, we use the following hack to only grab the spinlock if it
 * wasn't already owned by the process.
 */
static inline bool grab_pcbsdatalock_if_not_held(irqctx_t *pcbdata_ctx) {
    if (spinlock_trylock(&_laritos.proc.pcbs_data_lock, pcbdata_ctx)) {
        return true;
    } else if (!spinlock_owned_by_me(&_laritos.proc.pcbs_data_lock)) {
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, pcbdata_ctx);
        return true;
    }
    return false;
}

void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx) {
    pcb_t *pcb = process_get_current();

    irqctx_t pcbdata_ctx;
    bool pcbs_lock_acquired = grab_pcbsdatalock_if_not_held(&pcbdata_ctx);

    sched_move_to_blocked_locked(pcb, &cond->blocked);

    if (pcbs_lock_acquired) {
        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
    }
    spinlock_release(spin, ctx);

    verbose_async("pid=%u waiting for condition=0x%p", pcb->pid, cond);

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
    irqctx_t pcbdata_ctx;
    bool pcbs_lock_acquired = grab_pcbsdatalock_if_not_held(&pcbdata_ctx);

    pcb_t *pcb = list_first_entry_or_null(&cond->blocked, pcb_t, sched.sched_node);
    wakeup_pcb_locked(pcb, cond);

    if (pcbs_lock_acquired) {
        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
    }
    return pcb;
}

bool condition_notify_all_locked(condition_t *cond) {
    irqctx_t pcbdata_ctx;
    bool pcbs_lock_acquired = grab_pcbsdatalock_if_not_held(&pcbdata_ctx);

    pcb_t *pcb;
    pcb_t *tmp;
    bool proc_awakened = false;
    list_for_each_entry_safe(pcb, tmp, &cond->blocked, sched.sched_node) {
        wakeup_pcb_locked(pcb, cond);
        proc_awakened = true;
    }

    if (pcbs_lock_acquired) {
        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
    }

    return proc_awakened;
}
