#define DEBUG
#include <log.h>

#include <stdbool.h>

#include <cpu.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/assert.h>
#include <process/status.h>
#include <component/sched.h>

void sched_switch_to(pcb_t *from, pcb_t *to) {
    sched_move_to_running(to);

    if (from != NULL) {
        context_save_and_restore(from, to);
        // Once the from context is restored, it will continue execution from
        // here (actually from within the context_save_and_restore() function)
        verbose_async("Resuming execution of pid=%u", pcb_get_current()->pid);
    } else {
        context_restore(to);
        // Execution will never reach this point
    }
}

static void context_switch(pcb_t *cur, pcb_t *to) {
    if (cur != NULL) {
        verbose_async("Context switch pid=%u -> pid=%u", cur->pid, to->pid);
        pcb_set_current(NULL);
        // Check whether the process is actually running (i.e. not a zombie)
        if (cur->sched.status == PCB_STATUS_RUNNING) {
            sched_move_to_ready(cur);
        }
    }
    sched_switch_to(cur, to);
}

void schedule(void) {
#ifdef DEBUG
    pcb_t *proc;
    log_always("Processes:");
    for_each_process(proc) {
        log_always("   pid=%u, status=%s", proc->pid, pcb_get_status_str(proc->sched.status));
    }
#endif

    cpu_t *c = cpu();
    pcb_t *curpcb = pcb_get_current();
    pcb_t *pcb = c->sched->ops.pick_ready(c->sched, c, curpcb);

    assert(pcb != NULL, "No process ready for execution, where is the idle process?");
    if (curpcb != pcb) {
        context_switch(curpcb, pcb);
    }
}
