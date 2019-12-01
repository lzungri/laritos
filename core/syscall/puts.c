#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>

int syscall_puts(const char *s) {
    info_async("[pid=%u] %s", pcb_get_current()->pid, s);
    return 0;
}
