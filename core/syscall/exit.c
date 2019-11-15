#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>

void syscall_exit(int status) {
    pcb_t *pcb = pcb_get_current();
    info_async("Exiting process pid=%u, exitcode=%d", pcb->pid, status);
    pcb_kill_and_schedule(pcb);
}
