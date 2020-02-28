#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

int syscall_puts(const char *s) {
    // Output process message as a raw string (i.e. no date, pid, tag metadata, etc)
    __add_log_msg(true, NULL, NULL, (char *) s);
    return 0;
}
