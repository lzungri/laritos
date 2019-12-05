#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>
#include <sched/core.h>

int syscall_set_priority(uint8_t priority) {
    pcb_t *pcb = pcb_get_current();
    verbose_async("Setting process pid=%u priority to %u", pcb->pid, priority);
    return pcb_set_priority(pcb, priority);
}
