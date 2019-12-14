#define DEBUG
#include <log.h>

#include <stdbool.h>

#include <cpu.h>
#include <process/core.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/assert.h>
#include <utils/debug.h>
#include <process/status.h>
#include <component/sched.h>

void sched_switch_to(pcb_t *from, pcb_t *to) {
    sched_move_to_running(to);
#ifdef DEBUG
    debug_dump_processes();
#endif
    context_save_and_restore(from, to);

    // Once the *from* context is restored, it will continue execution from
    // here (actually from within the context_save_and_restore() function)
    verbose_async("Resuming execution of pid=%u", pcb_get_current()->pid);
}

static void context_switch(pcb_t *cur, pcb_t *to) {
    verbose_async("Context switch pid=%u -> pid=%u", cur->pid, to->pid);
    pcb_set_current(NULL);
    // Check whether the process is actually running (i.e. not a zombie)
    if (cur->sched.status == PCB_STATUS_RUNNING) {
        sched_move_to_ready(cur);
    }
    sched_switch_to(cur, to);
}

void schedule(void) {
    cpu_t *c = cpu();
    pcb_t *curpcb = pcb_get_current();
    pcb_t *pcb = c->sched->ops.pick_ready(c->sched, c, curpcb);

    // If the current process is running and:
    //      - there is no other pcb ready,
    //      - or there is another pcb ready but with lower priority (i.e. higher number),
    // then continue execution of the current process
    if (curpcb->sched.status == PCB_STATUS_RUNNING) {
        if (pcb == NULL || (curpcb->sched.priority < pcb->sched.priority)) {
            return;
        }
    }

    assert(pcb != NULL, "No process ready for execution, where is the idle process?");
    if (curpcb != pcb) {
        context_switch(curpcb, pcb);
    }
}

void sched_execute_first_system_proc(pcb_t *pcb) {
    // Execute the first process
    sched_move_to_running(pcb);

    // From now on, everything will be executed in the context of a process
    _laritos.process_mode = true;
    info("Process mode started");

    context_restore(pcb);
    // Execution will never reach this point
}
