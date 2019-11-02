#pragma once

#include <log.h>
#include <core.h>
#include <process/pcb.h>
#include <process/status.h>


static inline void sched_move_to_ready(pcb_t *pcb) {
    verbose("PID %u: %s -> READY", pcb->pid, get_pcb_status_str(pcb->status));
    // TODO Mutex
    pcb->status = PCB_STATUS_READY;
    list_add(&pcb->sched_node, &_laritos.ready_pcbs);
}

static inline void sched_remove_from_zombie(pcb_t *pcb) {
    if (pcb->status != PCB_STATUS_ZOMBIE) {
        return;
    }
    verbose("PID %u: %s -> NOT_INIT", pcb->pid, get_pcb_status_str(pcb->status));
    // TODO Mutex
    list_del(&pcb->sched_node);
    pcb->status = PCB_STATUS_NOT_INIT;
}

static inline void sched_move_to_zombie(pcb_t *pcb) {
    verbose("PID %u: %s -> ZOMBIE ", pcb->pid, get_pcb_status_str(pcb->status));
    // TODO Mutex
    list_del(&pcb->sched_node);
    list_add(&pcb->sched_node, &_laritos.zombie_pcbs);
    pcb->status = PCB_STATUS_ZOMBIE;
}
