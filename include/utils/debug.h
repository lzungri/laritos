#pragma once

#include <log.h>
#include <string.h>
#include <printf.h>
#include <core.h>
#include <component/component.h>
#include <component/intc.h>
#include <arch/debug.h>
#include <process/core.h>
#include <process/status.h>
#include <sched/context.h>
#include <sched/core.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>
#include <irq/core.h>
#include <time/tick.h>
#include <component/vrtimer.h>
#include <utils/math.h>
#include <syscall/syscall-no.h>

static inline void debug_message_delimiter(void) {
    error_async("*** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
}

static inline void debug_dump_kernel_stats(void) {
    log_always("Kernel stats:");
    log_always("  OS ticks | %lu", (uint32_t) tick_get_os_ticks());
    log_always("  switches | %lu", atomic32_get(&_laritos.stats.ctx_switches));

    int i;
    char buf[128] = { 0 };
    int written = 0;
    intc_t *intc = component_get_default(COMP_TYPE_INTC, intc_t);
    if (intc != NULL) {
        for (i = 0; i < ARRAYSIZE(intc->irq_count); i++) {
            written += snprintf(buf + written, sizeof(buf) - written,
                    "%d=%ld ", i, atomic32_get(&intc->irq_count[i]));
            if ((i + 1) % 10 == 0 || i == ARRAYSIZE(intc->irq_count) - 1) {
                written = 0;
                log_always("      irqs | %s", buf);
            }
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
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);

    for_each_process_locked(proc) {
        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

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

        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    }

    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);
}

static inline void _dump_pstree_for(pcb_t *pcb, uint8_t level) {
    char buf[128];
    uint8_t nspaces = min(sizeof(buf) - 1, level * 2);
    memset(buf, ' ', nspaces);

    if (level > 20) {
        strncpy(buf + nspaces, "...", sizeof(buf) - nspaces - 1);
        log_always("%s", buf);
        // To prevent stack overflows
        return;
    }

    strncpy(buf + nspaces, pcb->name, sizeof(buf) - nspaces - 1);
    log_always("%s", buf);

    pcb_t *child;
    for_each_child_process_locked(pcb, child) {
        _dump_pstree_for(child, level + 1);
    }
}

static inline void debug_dump_pstree(void) {
    log_always("pstree:");
    // Prevent the OS from doing any change on the active processes
    // WARNING: We can only do this here since this is only used for debugging purposes
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    _dump_pstree_for(_laritos.proc.init, 1);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
}

static inline void debug_dump_processes_stats(void) {
    pcb_t *proc;

    // Prevent the OS from doing any change on the active processes
    // WARNING: We can only do this here since this is only used for debugging purposes
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);

    log_always("Processes stats:");
    for_each_process_locked(proc) {
        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

        log_always("  %s (pid=%u)", proc->name, proc->pid);

        time_t t;
        time_get_monotonic_time(&t);
        time_sub(&t, &proc->sched.start_time, &t);
        uint16_t hours, mins, secs;
        time_to_hms(&t, &hours, &mins, &secs);
        log_always("      start | %02u:%02u:%02u", hours, mins, secs);

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
        sched_update_stats_locked(proc);
        log_always("      sched | ready=%lu running=%lu blocked=%lu zombie=%lu", proc->stats.ticks_spent[PROC_STATUS_READY], proc->stats.ticks_spent[PROC_STATUS_RUNNING],
                    proc->stats.ticks_spent[PROC_STATUS_BLOCKED], proc->stats.ticks_spent[PROC_STATUS_ZOMBIE]);

        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

        log_always("  -----");
    }

    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);
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
