#include <log.h>

#include <process/pcb.h>

pcb_t *sched_algo_pick_ready(pcb_t *curpcb) {
    pcb_t *pcb;
    for_each_ready_process(pcb) {
        return pcb;
    }
    return NULL;
}
