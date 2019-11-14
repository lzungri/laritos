#pragma once

#include <log.h>
#include <core.h>
#include <process/pcb.h>
#include <process/status.h>

void switch_to(pcb_t *cur, pcb_t *pcb);
void schedule(void);
pcb_t *sched_algo_pick_ready(pcb_t *curpcb);

static inline void sched_move_to_ready(pcb_t *pcb) {
    verbose_async("PID %u: %s -> READY", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_move_tail(&pcb->sched.sched_node, &_laritos.sched.ready_pcbs);
    pcb->sched.status = PCB_STATUS_READY;
}

static inline void sched_move_to_running(pcb_t *pcb) {
    verbose_async("PID %u: %s -> RUNNING", pcb->pid, pcb_get_status_str(pcb->sched.status));
    // TODO Mutex
    list_del_init(&pcb->sched.sched_node);
    pcb->sched.status = PCB_STATUS_RUNNING;
    pcb_set_current(pcb);
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
