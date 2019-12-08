#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>
#include <time/time.h>

int syscall_time(time_t *t) {
    return time_rtc_gettime(t);
}
