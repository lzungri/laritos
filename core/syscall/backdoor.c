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

#include <core.h>
#include <process/core.h>
#include <process/types.h>
#include <syscall/syscall.h>
#include <mm/heap.h>
#include <time/core.h>
#include <utils/debug.h>
#include <utils/utils.h>
#include <mm/slab.h>
#include <strtoxl.h>

typedef struct {
    char *cmd;
    int (*handler)(void *param);
} bdcmd_t;


static int bd_crash_undef_exception(void *param) {
    // undef exc
    asm(".word 0xffffffff");
    return 0;
}

static int bd_crash_data(void *param) {
    // data abort exc
    asm("movw r7, #0xffff");
    asm("movt r7, #0xffff");
    asm("str r6, [r7]");
    return 0;
}

static int bd_crash_prefetch(void *param) {
    // prefetch exc
    asm("movw r7, #0xffff");
    asm("movt r7, #0xffff");
    asm("mov pc, r7");
    return 0;
}

static int bd_crash_syscall(void *param) {
    // system call
    asm("mov r0, #1");
    asm("mov r1, #2");
    asm("mov r2, #3");
    asm("mov r3, #4");
    asm("mov r4, #5");
    asm("mov r5, #6");
    asm("mov r6, #7");
    asm("svc 1");
    return 0;
}

static int bd_crash_stack_prot(void *param) {
    pcb_t *proc = process_get_current();
    uint32_t *bottom = proc->mm.stack_bottom;
    *bottom = 0xCACACACA;
    uint32_t *top = (uint32_t *) proc->mm.stack_top;
    *top = 0xBABABABA;
    return 0;
}

static int bd_crash_heap_ov(void *param) {
    // Heap buffer overflow
    char *ptr = malloc(10);
    ptr[10] = 0xCA;
    ptr[11] = 0xCA;
    ptr[12] = 0xCA;
    ptr[13] = 0xCA;
    free(ptr);
    return 0;
}

static int bd_reset(void *param) {
    // reset
    asm("b 0");
    return 0;
}

static int bd_proc_stats(void *param) {
    debug_dump_processes();
    debug_dump_processes_stats();
    return 0;
}

static int bd_kernel_stats(void *param) {
    debug_dump_kernel_stats();
    return 0;
}

static int bd_pstree(void *param) {
    debug_dump_pstree();
    return 0;
}

static int bd_malloc(void *param) {
    if (param == NULL) {
        error("Syntax: bd malloc <size>");
        return -1;
    }
    unsigned long size = strtoul(param, NULL, 0);
    void *ptr = malloc(size);
    if (ptr == NULL) {
        error("Not enough memory");
        return -1;
    }
    info("new malloc'ed chunk of %lu bytes at %p", size, ptr);
    return 0;
}

static int bd_free(void *param) {
    if (param == NULL) {
        error("Syntax: bd free <ptr_in_hex>");
        return -1;
    }
    void *ptr = (void *) strtoul(param, NULL, 16);
    info("Freeing ptr=0x%p", ptr);
    free(ptr);
    return 0;
}

static int bd_debugpid(void *param) {
    if (param == NULL) {
        error("Syntax: bd debugpid <pid>");
        return -1;
    }

    int pid = (int) strtoul(param, NULL, 0);
    if (!slab_is_taken(_laritos.proc.pcb_slab, pid)) {
        error("pid=%u not found", pid);
        return -1;
    }

    pcb_t *pcb = slab_get_ptr_from_position(_laritos.proc.pcb_slab, pid);
    if (pcb != NULL) {
        info("Steps to debug %s:", pcb->cmd);
        info("  1- Make sure you launched the OS in debugging mode");
        info("  2- Interrupt gdb via ctrl-c");
        info("  3- Run this in your gdb session: add-symbol-file <path_to_elf> 0x%p", pcb->mm.text_start);
        info("  4- Setup breakpoints, watchpoints, etc, then type continue");
        return 0;
    }
    return -1;
}

static bdcmd_t commands[] = {
    { .cmd = "crash_undef", .handler = bd_crash_undef_exception },
    { .cmd = "crash_data", .handler = bd_crash_data },
    { .cmd = "crash_prefetch", .handler = bd_crash_prefetch },
    { .cmd = "crash_syscall", .handler = bd_crash_syscall },
    { .cmd = "crash_heap_ov", .handler = bd_crash_heap_ov },
    { .cmd = "crash_stack_prot", .handler = bd_crash_stack_prot },
    { .cmd = "reset", .handler = bd_reset },
    { .cmd = "pstats", .handler = bd_proc_stats },
    { .cmd = "kstats", .handler = bd_kernel_stats },
    { .cmd = "pstree", .handler = bd_pstree },
    { .cmd = "malloc", .handler = bd_malloc },
    { .cmd = "free", .handler = bd_free },
    { .cmd = "debugpid", .handler = bd_debugpid },
};

static void help_message(void) {
    info("Available commands:");
    int i;
    for (i = 0; i < ARRAYSIZE(commands); i++) {
        info("   %s", commands[i].cmd);
    }
}

int syscall_backdoor(char *command, void *arg) {
    if (command == NULL || command[0] == '\0') {
        error("Command cannot be NULL or empty");
        return -1;
    }

    int i;
    for (i = 0; i < ARRAYSIZE(commands); i++) {
        bdcmd_t *cmd = &commands[i];
        if (strncmp(command, cmd->cmd, 64) == 0) {
            return cmd->handler(arg);
        }
    }
    error("Command '%s' not recognized", command);
    help_message();
    return -1;
}
