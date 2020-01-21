#pragma once

#include <log.h>
#include <string.h>
#include <printf.h>
#include <core.h>
#include <component/component.h>
#include <arch/debug.h>
#include <process/core.h>
#include <process/status.h>
#include <sched/context.h>
#include <sched/core.h>
#include <sync/atomic.h>
#include <syscall/syscall-no.h>

static inline void debug_message_delimiter(void) {
    error_async("*** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
}

static inline void debug_dump_kernel_stats(void) {
    log_always("Kernel stats:");
    log_always("  switches | %lu", atomic32_get(&_laritos.stats.ctx_switches));

    int i;
    char buf[128] = { 0 };
    int written = 0;
    for (i = 0; i < ARRAYSIZE(_laritos.stats.nirqs); i++) {
        written += snprintf(buf + written, sizeof(buf) - written,
                "%d=%ld ", i, atomic32_get(&_laritos.stats.nirqs[i]));
        if ((i + 1) % 10 == 0 || i == ARRAYSIZE(_laritos.stats.nirqs) - 1) {
            written = 0;
            log_always("      irqs | %s", buf);
        }
    }
}

static inline void debug_dump_processes(void) {
    char buf[64] = { 0 };
    pcb_t *proc;
    log_always("Processes:");
    log_always("name    pid ppid type  status   prio       mode       pc          sp_ctx");

    // Prevent the OS from doing any change on the active processes
    // WARNING: We can only do this here since this is only used for debugging purposes
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proclock, &ctx);

    for_each_process(proc) {
        if (proc->sched.status == PROC_STATUS_RUNNING) {
            log_always("%-7.7s %2u  %2u    %s   %7s    %3u   %-12.12s   0x%p           -",
                    proc->name, proc->pid, proc->parent != NULL ? proc->parent->pid : 0, proc->kernel ? "K" : "U",
                    pcb_get_status_str(proc->sched.status), proc->sched.priority,
                    arch_debug_get_psr_str(arch_cpu_get_cpsr(), buf, sizeof(buf)), arch_cpu_get_pc());
        } else {
            log_always("%-7.7s %2u  %2u    %s   %7s    %3u   %-12.12s   0x%p  0x%p",
                    proc->name, proc->pid, proc->parent != NULL ? proc->parent->pid : 0, proc->kernel ? "K" : "U",
                    pcb_get_status_str(proc->sched.status), proc->sched.priority,
                    arch_debug_get_psr_str_from_ctx(proc->mm.sp_ctx, buf, sizeof(buf)),
                    arch_context_get_retaddr(proc->mm.sp_ctx), proc->mm.sp_ctx);
        }
    }

    spinlock_release(&_laritos.proclock, &ctx);
}

static inline void debug_dump_processes_stats(void) {
    pcb_t *proc;

    // Prevent the OS from doing any change on the active processes
    // WARNING: We can only do this here since this is only used for debugging purposes
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proclock, &ctx);

    log_always("Processes stats:");
    for_each_process(proc) {
        log_always("  %s (pid=%u)", proc->name, proc->pid);

        // Number of syscalls stats
        int i;
        char buf[128] = { 0 };
        int written = 0;
        for (i = 0; i < SYSCALL_LEN; i++) {
            written += snprintf(buf + written, sizeof(buf) - written,
                    "%d=%ld ", i, atomic32_get(&proc->stats.syscalls[i]));
            if ((i + 1) % 10 == 0 || i == SYSCALL_LEN - 1) {
                written = 0;
                log_always("    syscall | %s", buf);
            }
        }

        // Scheduling stats
        // Update with the latest stats
        sched_update_stats(proc);
        log_always("      sched | ready=%lu running=%lu blocked=%lu zombie=%lu", proc->stats.ticks_spent[PROC_STATUS_READY], proc->stats.ticks_spent[PROC_STATUS_RUNNING],
                    proc->stats.ticks_spent[PROC_STATUS_BLOCKED], proc->stats.ticks_spent[PROC_STATUS_ZOMBIE]);
        log_always("  -----");
    }

    spinlock_release(&_laritos.proclock, &ctx);
}

/**
 * Print the current state of the OS
 *
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful PC/LR, not just the PC inside the dump_cur_state() function (in case it
 * wasn't expanded by the compiler)
 */
__attribute__((always_inline)) static inline void debug_dump_cur_state(void) {
    // IMPORTANT: Make sure you don't change any relevant registers before
    // calling dump_all_regs()
    arch_debug_dump_all_regs();
    debug_dump_kernel_stats();
    // TODO: Dump kernel info
    void heap_dump_info(void);
    heap_dump_info();
    debug_dump_processes();
    debug_dump_processes_stats();
}

static inline void debug_dump_registered_comps(void) {
    component_t *c;
    log_always("Components:");
    for_each_component(c) {
        log_always("   %s@0x%p, type: %d", c->id, c, c->type);
        if (strnlen(c->product, sizeof(c->product)) > 0) {
            log_always("      product: %s", c->product);
        }
        if (strnlen(c->vendor, sizeof(c->vendor)) > 0) {
            log_always("      vendor: %s", c->vendor);
        }
        if (strnlen(c->description, sizeof(c->description)) > 0) {
            log_always("      description: %s", c->description);
        }
    }
}
