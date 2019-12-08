#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_set_process_name(char *name) {
    pcb_t *pcb = pcb_get_current();
    pcb_set_name(pcb, name);
    return 0;
}
