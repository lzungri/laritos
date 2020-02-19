#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_puts(const char *s) {
    info("%s", s);
    return 0;
}
