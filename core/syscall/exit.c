#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>

void syscall_exit(int status) {
    pcb_t *pcb = pcb_get_current();
    pcb->exit_status = status;
    info_async("Exiting process pid=%u, exitcode=%d", pcb->pid, pcb->exit_status);
    pcb_kill_and_schedule(pcb);
}
