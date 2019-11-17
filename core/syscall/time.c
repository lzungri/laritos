#include <log.h>

#include <process/pcb.h>
#include <syscall/syscall.h>
#include <time/time.h>

int syscall_time(time_t *t) {
    return rtc_gettime(t);
}
