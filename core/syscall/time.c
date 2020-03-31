/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
