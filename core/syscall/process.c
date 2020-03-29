#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>
#include <sched/core.h>
#include <loader/loader.h>

int syscall_set_priority(uint8_t priority) {
    pcb_t *pcb = process_get_current();
    verbose_async("Setting process pid=%u priority to %u", pcb->pid, priority);
    return process_set_priority(pcb, priority);
}

int syscall_set_process_name(char *name) {
    pcb_t *pcb = process_get_current();
    process_set_name(pcb, name);
    return 0;
}

int syscall_getpid(void) {
    return process_get_current()->pid;
}

void syscall_exit(int status) {
    process_exit(status);
    // Execution will never reach this point
}

int syscall_spawn_process(char *executable) {
    pcb_t *pcb = loader_load_executable_from_file(executable);
    return pcb == NULL ? -1 : pcb->pid;
}

int syscall_waitpid(int pid, int *status) {
    return process_wait_pid(pid, status);
}
