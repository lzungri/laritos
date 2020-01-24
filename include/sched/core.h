#pragma once

#include <log.h>
#include <core.h>
#include <cpu/cpu-local.h>
#include <process/core.h>
#include <process/status.h>
#include <sync/condition.h>
#include <time/system-tick.h>
#include <irq/core.h>

void sched_switch_to(pcb_t *cur, pcb_t *pcb);
void schedule(void);
void sched_execute_first_system_proc(pcb_t *pcb);

static inline void schedule_if_needed(void) {
    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);

    // Check whether we need to schedule
    if (_laritos.sched.need_sched && _laritos.process_mode) {
        insane_async("Re-schedule needed");
        _laritos.sched.need_sched = false;
        schedule();
    }

    irq_local_restore_ctx(&ctx);
}

static inline void sched_update_stats(pcb_t *pcb) {
    tick_t delta = tick_get_os_ticks() - pcb->stats.last_status_change;
    pcb->stats.ticks_spent[pcb->sched.status] += delta;
    pcb->stats.last_status_change = tick_get_os_ticks();
}

/**
 * Adds a READY process in the ready queue sorted in ascending order by priority number
 * (i.e. highest priority process are first in the list).
 * If two or more processes share the same priority, the new pcb will be added next to the
 * last process with that priority.
 *
 * @param pcb: Process to add
 * @return void
 */
static inline void _sched_add_ready_proc_sorted(pcb_t *pcb) {
    list_del_init(&pcb->sched.sched_node);
    pcb_t *proc;
    for_each_ready_process(proc) {
        if (pcb->sched.priority < proc->sched.priority) {
            list_add(&pcb->sched.sched_node, proc->sched.sched_node.prev);
            return;
        }
    }
    list_add_tail(&pcb->sched.sched_node, CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs));
}

static inline void sched_move_to_ready_locked(pcb_t *pcb) {
    if (pcb->sched.status == PROC_STATUS_ZOMBIE) {
        error_async("Cannot move a ZOMBIE process to READY");
        return;
    }
    insane_async("PID %u: %s -> READY", pcb->pid, pcb_get_status_str(pcb->sched.status));

    sched_update_stats(pcb);

    _sched_add_ready_proc_sorted(pcb);
    pcb->sched.status = PROC_STATUS_READY;

    // Re-schedule in case there is a new higher priority process
    if (list_first_entry(CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), pcb_t, sched.sched_node) == pcb) {
        _laritos.sched.need_sched = true;
    }
}

static inline void sched_move_to_blocked_locked(pcb_t *pcb) {
    if (pcb->sched.status == PROC_STATUS_ZOMBIE || pcb->sched.status == PROC_STATUS_NOT_INIT) {
        error_async("Cannot move a ZOMBIE or NOT_INIT process to BLOCKED");
        return;
    }
    insane_async("PID %u: %s -> BLOCKED", pcb->pid, pcb_get_status_str(pcb->sched.status));

    sched_update_stats(pcb);

    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PROC_STATUS_BLOCKED;
}

static inline void sched_move_to_running_locked(pcb_t *pcb) {
    if (pcb->sched.status != PROC_STATUS_READY) {
        error_async("Cannot move a non READY process to RUNNING");
        return;
    }
    insane_async("PID %u: %s -> RUNNING", pcb->pid, pcb_get_status_str(pcb->sched.status));

    sched_update_stats(pcb);

    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PROC_STATUS_RUNNING;
    process_set_current(pcb);
}

static inline void sched_move_to_zombie_locked(pcb_t *pcb) {
    insane_async("PID %u: %s -> ZOMBIE ", pcb->pid, pcb_get_status_str(pcb->sched.status));

    sched_update_stats(pcb);

    pcb_t *child;
    pcb_t *temp;
    pcb_t *gparent = pcb->parent;
    // Grandparent will become the new parent
    for_each_child_process_safe(pcb, child, temp) {
        insane_async("pid=%u new parent pid=%u", child->pid, gparent->pid);
        child->parent = gparent;
        list_move_tail(&child->siblings, &gparent->children);
    }

    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PROC_STATUS_ZOMBIE;

    process_release_zombie_resources(pcb);

    // Notify blocked parent (if any) about its dead
    condition_notify_proclocked(&pcb->parent_waiting_cond);

    if (gparent == _laritos.proc.init) {
        // New zombie process child of init, wake up init so that it releases its resources
        sched_move_to_ready_locked(_laritos.proc.init);
    }
}
