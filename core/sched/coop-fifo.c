#include <log.h>

#include <board/board.h>
#include <process/core.h>
#include <sched/core.h>
#include <component/sched.h>
#include <mm/heap.h>
#include <cpu/cpu-local.h>

static inline pcb_t *pick_ready_locked(sched_comp_t *sched, struct cpu *cpu, pcb_t *curpcb) {
    return list_first_entry_or_null(CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), pcb_t, sched.sched_node);
}

static int process(board_comp_t *comp) {
    sched_comp_t *s = component_alloc(sizeof(sched_comp_t));
    if (s == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    s->ops.pick_ready_locked = pick_ready_locked;

    if (component_init((component_t *) s, comp->id, comp, COMP_TYPE_SCHED, NULL, NULL) < 0) {
        error("Failed to initialize '%s' scheduler component", comp->id);
        goto fail;
    }

    component_set_info((component_t *) s, "FIFO Scheduler", "lzungri", "FIFO cooperative scheduler");

    if (component_register((component_t *) s) < 0) {
        error("Couldn't register scheduler '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(s);
    return -1;
}

DEF_DRIVER_MANAGER(coop_fifo, process);
