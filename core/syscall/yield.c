#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>
#include <sched/core.h>

int syscall_yield(void) {
    info_async("Yielding process pid=%u", pcb_get_current()->pid);
    schedule();
    return 0;
}
