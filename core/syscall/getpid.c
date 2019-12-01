#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>

int syscall_getpid(void) {
    return pcb_get_current()->pid;
}
