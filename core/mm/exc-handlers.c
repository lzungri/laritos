#include <log.h>

#include <stdint.h>
#include <cpu/core.h>
#include <sched/context.h>
#include <mm/exc-handlers.h>
#include <process/core.h>
#include <process/status.h>
#include <arch/context-types.h>
#include <utils/debug.h>
#include <utils/math.h>
#include <sched/core.h>
#include <arch/debug.h>
#include <sync/spinlock.h>
#include <irq/core.h>

void exc_dump_process_info_locked(pcb_t *pcb) {
    char buf[64] = { 0 };
    char buf2[64] = { 0 };
    error_async("pid=%u, name=%s, type=%s, status=%s, spsr=%s, cpsr=%s",
            pcb->pid, pcb->name, pcb->kernel ? "K" : "U",
            pcb_get_status_str(pcb->sched.status),
            arch_debug_get_psr_str(arch_cpu_get_saved_psr(), buf, sizeof(buf)),
            arch_debug_get_psr_str(arch_cpu_get_cpsr(), buf2, sizeof(buf2)));
}

void exc_dump_process_info(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    exc_dump_process_info_locked(pcb);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
}

void exc_handle_process_exception(pcb_t *pcb) {
    exc_dump_process_info(pcb);
    debug_message_delimiter();
    error("ABORT");
    // Kill the offending process
    process_kill(pcb);

    // If the exception occurred while in IRQ mode, then stop the kernel with a fatal error.
    // Otherwise, schedule a new process
    if (arch_cpu_is_irq_mode(arch_cpu_get_saved_psr())) {
        fatal("Error while handling irq");
    } else {
        // Switch to another ready process
        schedule();
        // Execution will never reach this point
    }
}

void exc_handle_generic_process_error(pcb_t *pcb, spctx_t *ctx) {
    debug_message_delimiter();

    int32_t pc = ctx->ret - 4;
    arch_debug_dump_regs(ctx->r, ARRAYSIZE(ctx->r) - 1, pc, ctx->ret, ctx->spsr);

    uint32_t *ptr = (uint32_t *) max(pc - 4 * 8, 0);
    error_async("Instructions around pc=0x%p:", (void *) pc);
    int i;
    for (i = 0; i < 16; i++, ptr++) {
        error_async(" %s [0x%p] 0x%08lx", ptr == (uint32_t *) pc ? "->" : "  ", ptr, *ptr);
    }

    exc_handle_process_exception(pcb);
}

void exc_undef_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    exc_handle_process_exception(process_get_current());
}

void exc_prefetch_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    exc_handle_process_exception(process_get_current());
}

void exc_abort_handler(int32_t pc, spctx_t *ctx) {
    if (!_laritos.process_mode) {
        fatal("ABORT");
    }

    process_set_current_pcb_stack_context(ctx);
    exc_handle_process_exception(process_get_current());
}
