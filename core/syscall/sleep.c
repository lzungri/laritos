#include <log.h>

#include <stdint.h>
#include <process/pcb.h>
#include <syscall/syscall.h>
#include <sched/core.h>
#include <time/time.h>

int syscall_sleep(uint32_t secs) {
    info_async("Putting process with pid=%u to sleep for %lu seconds", pcb_get_current()->pid, secs);
    sleep(secs);
    return 0;
}

int syscall_msleep(uint32_t msecs) {
    info_async("Putting process with pid=%u to sleep for %lu mseconds", pcb_get_current()->pid, msecs);
    msleep(msecs);
    return 0;
}

int syscall_usleep(uint32_t usecs) {
    info_async("Putting process with pid=%u to sleep for %lu useconds", pcb_get_current()->pid, usecs);
    usleep(usecs);
    return 0;
}
