#include <log.h>

#include <syscall/syscall.h>
#include <time/core.h>

int syscall_time(time_t *t) {
    return time_get_rtc_time(t);
}
