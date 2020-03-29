#include <log.h>

#include <syscall/syscall.h>
#include <sched/core.h>
#include <process/core.h>
#include <time/core.h>

int syscall_time(time_t *t) {
    return time_get_rtc_time(t);
}

int syscall_sleep(uint32_t secs) {
    info_async("Putting process with pid=%u to sleep for %lu seconds", process_get_current()->pid, secs);
    sleep(secs);
    return 0;
}

int syscall_msleep(uint32_t msecs) {
    info_async("Putting process with pid=%u to sleep for %lu mseconds", process_get_current()->pid, msecs);
    msleep(msecs);
    return 0;
}

int syscall_usleep(uint32_t usecs) {
    info_async("Putting process with pid=%u to sleep for %lu useconds", process_get_current()->pid, usecs);
    usleep(usecs);
    return 0;
}
