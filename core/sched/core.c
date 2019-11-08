#include <log.h>

#include <process/pcb.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/assert.h>

void switch_to(pcb_t *pcb) {
    sched_move_to_running(pcb);
    context_restore(pcb);
}

void context_switch(pcb_t *cur, pcb_t *to) {
    if (cur != NULL) {
        verbose_async("Context switch pid=%u -> pid=%u", cur->pid, to->pid);
        pcb_set_current(NULL);
        // Check whether the process is actually running (i.e. not a zombie)
        if (cur->sched.status == PCB_STATUS_RUNNING) {
            sched_move_to_ready(cur);
        }
        context_save(cur, 0);
    }
    switch_to(to);
}

void schedule(void) {
    pcb_t *curpcb = pcb_get_current();
    pcb_t *pcb = sched_algo_pick_ready(curpcb);

    assert(pcb != NULL, "No process ready for execution, where is the idle process?");
    if (curpcb != pcb) {
        context_switch(curpcb, pcb);
    }
}
