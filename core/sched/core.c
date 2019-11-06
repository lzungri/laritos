#include <log.h>

#include <process/pcb.h>
#include <sched/core.h>
#include <sched/context.h>

void switch_to(pcb_t *pcb) {
    sched_move_to_running(pcb);
    restore_context(pcb);
}

void context_switch(pcb_t *cur, pcb_t *to) {
    if (cur != NULL && cur != to) {
        pcb_set_current(NULL);
        sched_move_to_ready(cur);
    }
    switch_to(to);
}

void schedule(void) {
    pcb_t *curpcb = pcb_get_current();
    pcb_t *pcb = sched_algo_pick_ready(curpcb);
    return context_switch(curpcb, pcb);
}
