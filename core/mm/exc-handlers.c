#include <log.h>

#include <stdint.h>
#include <cpu.h>
#include <sched/context.h>
#include <mm/exc-handlers.h>
#include <process/core.h>
#include <process/status.h>
#include <arch/context-types.h>
#include <utils/debug.h>
#include <utils/math.h>
#include <sched/core.h>
#include <arch/debug.h>

static inline void dump_process_info(pcb_t *pcb) {
    char buf[64] = { 0 };
    char buf2[64] = { 0 };
    error_async("pid=%u, name=%s, type=%s, status=%s, spsr=%s, cpsr=%s",
            pcb->pid, pcb->name, pcb->kernel ? "K" : "U",
            pcb_get_status_str(pcb->sched.status),
            arch_get_psr_str(arch_get_saved_psr(), buf, sizeof(buf)),
            arch_get_psr_str(arch_get_cpsr(), buf2, sizeof(buf2)));
}

void handle_process_exception(pcb_t *pcb) {
    dump_process_info(pcb);
    debug_message_delimiter();
    error_async("ABORT");
    // Kill the offending process
    process_kill(pcb);

    // If the exception occurred while in IRQ mode, then stop the kernel with a fatal error.
    // Otherwise, schedule a new process
    if (arch_is_irq(arch_get_saved_psr())) {
        fatal("Error while handling irq");
    } else {
        // Switch to another ready process
        schedule();
        // Execution will never reach this point
    }
}

void exc_undef_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    handle_process_exception(process_get_current());
}

void exc_prefetch_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    handle_process_exception(process_get_current());
}

void exc_abort_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    handle_process_exception(process_get_current());
}
