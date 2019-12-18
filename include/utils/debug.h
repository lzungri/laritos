#pragma once

#include <log.h>
#include <string.h>
#include <component/component.h>
#include <arch/debug.h>
#include <process/core.h>
#include <process/status.h>
#include <sched/context.h>

static inline void debug_message_delimiter(void) {
    error_async("*** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
}

static inline void debug_dump_processes(void) {
    char buf[64] = { 0 };
    pcb_t *proc;
    log_always("Processes:");
    log_always("name      pid  type  status   prio       mode       pc          sp_ctx");
    for_each_process(proc) {
        log_always("%-8.8s  %3u   %s   %7s    %3u   %-12.12s   0x%p  0x%p",
                proc->name, proc->pid, proc->kernel ? "K" : "U",
                pcb_get_status_str(proc->sched.status), proc->sched.priority,
                arch_get_psr_str_from_ctx(proc->mm.sp_ctx, buf, sizeof(buf)),
                arch_context_get_retaddr(proc->mm.sp_ctx), proc->mm.sp_ctx);
    }
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
    arch_dump_all_regs();
    // TODO: Dump kernel info
    void heap_dump_info(void);
    heap_dump_info();
    debug_dump_processes();
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
