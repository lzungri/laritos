#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <board.h>
#include <process/core.h>
#include <sched/core.h>
#include <component/ticker.h>
#include <component/sched.h>
#include <mm/heap.h>

static inline pcb_t *pick_ready(sched_comp_t *sched, struct cpu *cpu, pcb_t *curpcb) {
    return list_first_entry_or_null(&_laritos.sched.ready_pcbs, pcb_t, sched.sched_node);
}

static int rr_ticker_cb(ticker_comp_t *t, void *data) {
    _laritos.sched.need_sched = true;
    return 0;
}

static int init(component_t *c) {
    sched_comp_t *sched = (sched_comp_t *) c;
    return sched->ticker->ops.add_callback(sched->ticker, rr_ticker_cb, sched);
}

static int deinit(component_t *c) {
    sched_comp_t *sched = (sched_comp_t *) c;
    return sched->ticker->ops.remove_callback(sched->ticker, rr_ticker_cb, sched);
}

static int process(board_comp_t *comp) {
    sched_comp_t *s = component_alloc(sizeof(sched_comp_t));
    if (s == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    s->ops.pick_ready = pick_ready;

    if (component_init((component_t *) s, comp->id, comp, COMP_TYPE_SCHED, init, deinit) < 0) {
        error("Failed to initialize '%s' scheduler component", comp->id);
        goto fail;
    }

    if (board_get_component_attr(comp, "ticker", (component_t **) &s->ticker) < 0) {
        error("Invalid or no ticker specified in the board info");
        goto fail;
    }

    component_set_info((component_t *) s, "RR Scheduler", "lzungri", "RR preemptive scheduler");

    if (component_register((component_t *) s) < 0) {
        error("Couldn't register scheduler '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(s);
    return -1;
}

DEF_DRIVER_MANAGER(preempt_rr, process);
