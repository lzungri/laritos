#include <log.h>

#include <stdint.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <sched/core.h>
#include <time/core.h>

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
