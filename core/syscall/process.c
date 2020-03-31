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

#include <process/core.h>
#include <syscall/syscall.h>
#include <sched/core.h>
#include <loader/loader.h>

int syscall_set_priority(uint8_t priority) {
    pcb_t *pcb = process_get_current();
    verbose_async("Setting process pid=%u priority to %u", pcb->pid, priority);
    return process_set_priority(pcb, priority);
}

int syscall_set_process_name(char *name) {
    pcb_t *pcb = process_get_current();
    process_set_name(pcb, name);
    return 0;
}

int syscall_getpid(void) {
    return process_get_current()->pid;
}

void syscall_exit(int status) {
    process_exit(status);
    // Execution will never reach this point
}

int syscall_spawn_process(char *executable) {
    pcb_t *pcb = loader_load_executable_from_file(executable);
    return pcb == NULL ? -1 : pcb->pid;
}

int syscall_waitpid(int pid, int *status) {
    return process_wait_pid(pid, status);
}
