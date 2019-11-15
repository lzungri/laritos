#define DEBUG
#include <log.h>

#include <dstruct/list.h>
#include <cpu.h>
#include <core.h>
#include <mm/slab.h>
#include <process/status.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <generated/autoconf.h>

int pcb_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.proc.pcbs);
    INIT_LIST_HEAD(&_laritos.sched.ready_pcbs);
    INIT_LIST_HEAD(&_laritos.sched.blocked_pcbs);
    INIT_LIST_HEAD(&_laritos.sched.zombie_pcbs);

    _laritos.proc.pcb_slab = slab_create(CONFIG_PROCESS_MAX_CONCURRENT_PROCS, sizeof(pcb_t));
    return _laritos.proc.pcb_slab != NULL ? 0 : -1;
}

int pcb_deinit_global_context(void) {
    slab_destroy(_laritos.proc.pcb_slab);
    return 0;
}

void pcb_assign_pid(pcb_t *pcb) {
    pcb->pid = (uint16_t) slab_get_slab_position(_laritos.proc.pcb_slab, pcb);
}

pcb_t *pcb_alloc(void) {
    pcb_t *pcb = slab_alloc(_laritos.proc.pcb_slab);
    if (pcb != NULL) {
        memset(pcb, 0, sizeof(pcb_t));
        INIT_LIST_HEAD(&pcb->sched.pcb_node);
        INIT_LIST_HEAD(&pcb->sched.sched_node);
        pcb->sched.status = PCB_STATUS_NOT_INIT;
    }
    return pcb;
}

int pcb_free(pcb_t *pcb) {
    if (pcb->sched.status != PCB_STATUS_NOT_INIT) {
        error_async("You can only free a process in PCB_STATUS_NOT_INIT state");
        return -1;
    }
    verbose_async("Freeing process with pid=%u", pcb->pid);
    slab_free(_laritos.proc.pcb_slab, pcb);
    return 0;
}

int pcb_register(pcb_t *pcb) {
    pcb_assign_pid(pcb);
    debug_async("Registering process with pid=%u", pcb->pid);
    // TODO Mutex
    list_add(&pcb->sched.pcb_node, &_laritos.proc.pcbs);
    sched_move_to_ready(pcb);
    return 0;
}

int pcb_unregister(pcb_t *pcb) {
    if (pcb->sched.status != PCB_STATUS_NOT_INIT && pcb->sched.status != PCB_STATUS_ZOMBIE) {
        error_async("You can only unregister a process in either PCB_STATUS_NOT_INIT or PCB_STATUS_ZOMBIE state");
        return -1;
    }
    debug_async("Un-registering process with pid=%u", pcb->pid);
    // TODO Mutex
    list_del(&pcb->sched.pcb_node);
    if (pcb->sched.status == PCB_STATUS_ZOMBIE) {
        sched_remove_from_zombie(pcb);
    }
    return pcb_free(pcb);
}

spctx_t *pcb_get_current_pcb_stack_context(void) {
    pcb_t *pcb = pcb_get_current();
    return pcb != NULL ? pcb->mm.sp_ctx : 0;
}

void pcb_kill(pcb_t *pcb) {
    verbose_async("Killing process pid=%u", pcb->pid);
    sched_move_to_zombie(pcb);
}

void pcb_kill_and_schedule(pcb_t *pcb) {
    pcb_kill(pcb);
    schedule();
}
