#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_set_process_name(char *name) {
    pcb_t *pcb = process_get_current();
    process_set_name(pcb, name);
    return 0;
}
