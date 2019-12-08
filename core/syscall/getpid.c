#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_getpid(void) {
    return pcb_get_current()->pid;
}
