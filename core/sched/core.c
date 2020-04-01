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

//#define DEBUG
#include <log.h>

#include <stdbool.h>

#include <cpu/core.h>
#include <process/core.h>
#include <process/validation.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/assert.h>
#include <utils/debug.h>
#include <process/status.h>
#include <component/sched.h>
#include <mm/spprot.h>
#include <mm/exc-handlers.h>
#include <sync/spinlock.h>
#include <sync/atomic.h>

/**
 * NOTE: Must be called with irqs disabled and pcbs_data_lock held
 */
static inline void sched_switch_to_locked(pcb_t *from, pcb_t *to, irqctx_t *pcbdatalock_ctx) {
    sched_move_to_running_locked(to);

    spinlock_release(&_laritos.proc.pcbs_data_lock, pcbdatalock_ctx);

    context_save_and_restore(from, to);

    // Once the *from* context is restored, it will continue execution from
    // here (actually from within the context_save_and_restore() function)

    insane_async("Resuming execution of pid=%u", process_get_current()->pid);
    // Check if the stack has been corrupted
    if (spprot_is_stack_corrupted(process_get_current())) {
        // Kill process and schedule
        exc_handle_process_exception(process_get_current());
        // Execution will never reach this point
    }

#ifdef DEBUG
    debug_dump_processes();
#endif
}

/**
 * NOTE: Must be called with irqs disabled and pcbs_data_lock held
 */
static inline void context_switch_locked(pcb_t *cur, pcb_t *to, irqctx_t *pcbdatalock_ctx) {
    insane_async("Context switch pid=%u -> pid=%u", cur->pid, to->pid);

    // Update context switch stats
    atomic32_inc(&_laritos.stats.ctx_switches);

    // Check whether the process is actually running (i.e. not a zombie)
    if (cur->sched.status == PROC_STATUS_RUNNING) {
        sched_move_to_ready_locked(cur);
    }

    sched_switch_to_locked(cur, to, pcbdatalock_ctx);
}

void schedule(void) {
    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);

    cpu_t *c = cpu();
    pcb_t *curpcb = process_get_current();

    // This lock is going to be released right before performing the actual context switch in
    // sched_switch_to_locked()
    irqctx_t pcbdatalock_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);

    pcb_t *pcb = c->sched->ops.pick_ready_locked(c->sched, c, curpcb);;
    while (pcb != NULL && pcb != curpcb && !process_is_valid_context_locked(pcb, pcb->mm.sp_ctx)) {
        exc_dump_process_info_locked(pcb);
        error_async("Cannot switch to pid=%u, invalid context. Killing process...", pcb->pid);
        process_kill_locked(pcb);
        pcb = c->sched->ops.pick_ready_locked(c->sched, c, curpcb);;
    }

    // If the current process is running and:
    //      - there is no other pcb ready,
    //      - or there is another pcb ready but with lower priority (i.e. higher number),
    // then continue execution of the current process

    if (curpcb->sched.status == PROC_STATUS_RUNNING) {
        if (pcb == NULL || (curpcb->sched.priority < pcb->sched.priority)) {
            spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);
            irq_local_restore_ctx(&ctx);
            return;
        }
    }

    assert(pcb != NULL, "No process ready for execution, where is the idle process?");
    if (curpcb != pcb) {
        context_switch_locked(curpcb, pcb, &pcbdatalock_ctx);
    } else {
        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);
    }

    irq_local_restore_ctx(&ctx);
}

void sched_execute_first_system_proc(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    // Execute the first process
    sched_move_to_running_locked(pcb);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    context_restore(pcb);
    // Execution will never reach this point
}
