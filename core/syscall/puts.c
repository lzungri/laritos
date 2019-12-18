#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_puts(const char *s) {
    info_async("[pid=%u] %s", process_get_current()->pid, s);
    return 0;
}
