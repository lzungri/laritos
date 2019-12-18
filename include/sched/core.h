#pragma once

#include <log.h>
#include <core.h>
#include <process/core.h>
#include <process/status.h>

void sched_switch_to(pcb_t *cur, pcb_t *pcb);
void schedule(void);
void sched_execute_first_system_proc(pcb_t *pcb);

static inline void schedule_if_needed(void) {
    // Check whether we need to schedule
    if (_laritos.sched.need_sched && _laritos.process_mode) {
        verbose_async("Re-schedule needed");
        _laritos.sched.need_sched = false;
        schedule();
    }
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
static inline void sched_add_ready_proc_sorted(pcb_t *pcb) {
    list_del_init(&pcb->sched.sched_node);
    pcb_t *proc;
    for_each_ready_process(proc) {
        if (pcb->sched.priority < proc->sched.priority) {
            list_add(&pcb->sched.sched_node, proc->sched.sched_node.prev);
            return;
        }
    }
    list_add_tail(&pcb->sched.sched_node, &_laritos.sched.ready_pcbs);
}

static inline void sched_move_to_ready(pcb_t *pcb) {
    verbose_async("PID %u: %s -> READY", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    sched_add_ready_proc_sorted(pcb);
    pcb->sched.status = PCB_STATUS_READY;

    // Re-schedule in case there is a new higher priority process
    if (list_first_entry(&_laritos.sched.ready_pcbs, pcb_t, sched.sched_node) == pcb) {
        _laritos.sched.need_sched = true;
    }
}

static inline void sched_move_to_blocked(pcb_t *pcb) {
    verbose_async("PID %u: %s -> BLOCKED", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_move_tail(&pcb->sched.sched_node, &_laritos.sched.blocked_pcbs);
    pcb->sched.status = PCB_STATUS_BLOCKED;
}

static inline void sched_move_to_running(pcb_t *pcb) {
    verbose_async("PID %u: %s -> RUNNING", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PCB_STATUS_RUNNING;
    process_set_current(pcb);
}

static inline void sched_remove_from_zombie(pcb_t *pcb) {
    if (pcb->sched.status != PCB_STATUS_ZOMBIE) {
        return;
    }
    verbose_async("PID %u: %s -> NOT_INIT", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PCB_STATUS_NOT_INIT;
}

static inline void sched_move_to_zombie(pcb_t *pcb) {
    verbose_async("PID %u: %s -> ZOMBIE ", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_move_tail(&pcb->sched.sched_node, &_laritos.sched.zombie_pcbs);
    pcb->sched.status = PCB_STATUS_ZOMBIE;
}
