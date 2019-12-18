#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

void syscall_exit(int status) {
    pcb_t *pcb = process_get_current();
    pcb->exit_status = status;
    info_async("Exiting process pid=%u, exitcode=%d", pcb->pid, pcb->exit_status);
    process_kill_and_schedule(pcb);
}
