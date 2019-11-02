#include <log.h>

#include <dstruct/list.h>
#include <cpu.h>
#include <core.h>
#include <mm/slab.h>
#include <process/status.h>
#include <sched/sched.h>
#include <process/pcb.h>
#include <generated/autoconf.h>

int pcb_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.pcbs);
    INIT_LIST_HEAD(&_laritos.ready_pcbs);
    INIT_LIST_HEAD(&_laritos.blocked_pcbs);
    INIT_LIST_HEAD(&_laritos.zombie_pcbs);

    _laritos.pcb_slab = slab_create(CONFIG_PROCESS_MAX_CONCURRENT_PROCS, sizeof(pcb_t));
    return _laritos.pcb_slab != NULL ? 0 : -1;
}

int pcb_deinit_global_context(void) {
    slab_destroy(_laritos.pcb_slab);
    return 0;
}

void pcb_assign_pid(pcb_t *pcb) {
    pcb->pid = (uint16_t) slab_get_slab_position(_laritos.pcb_slab, pcb);
}

pcb_t *pcb_alloc(void) {
    pcb_t *pcb = slab_alloc(_laritos.pcb_slab);
    if (pcb != NULL) {
        memset(pcb, 0, sizeof(pcb_t));
        INIT_LIST_HEAD(&pcb->pcb_node);
        INIT_LIST_HEAD(&pcb->sched_node);
        pcb->status = PCB_STATUS_NOT_INIT;
    }
    return pcb;
}

int pcb_free(pcb_t *pcb) {
    if (pcb->status != PCB_STATUS_NOT_INIT) {
        error("You can only free a process in PCB_STATUS_NOT_INIT state");
        return -1;
    }
    verbose("Freeing process with pid=%u", pcb->pid);
    slab_free(_laritos.pcb_slab, pcb);
    return 0;
}

int pcb_register(pcb_t *pcb) {
    pcb_assign_pid(pcb);
    debug("Registering process with pid=%u", pcb->pid);
    // TODO Mutex
    list_add(&pcb->pcb_node, &_laritos.pcbs);
    sched_move_to_ready(pcb);
    return 0;
}

int pcb_unregister(pcb_t *pcb) {
    if (pcb->status != PCB_STATUS_NOT_INIT && pcb->status != PCB_STATUS_ZOMBIE) {
        error("You can only unregister a process in either PCB_STATUS_NOT_INIT or PCB_STATUS_ZOMBIE state");
        return -1;
    }
    debug("Un-registering process with pid=%u", pcb->pid);
    // TODO Mutex
    list_del(&pcb->pcb_node);
    if (pcb->status == PCB_STATUS_ZOMBIE) {
        sched_remove_from_zombie(pcb);
    }
    return pcb_free(pcb);
}
