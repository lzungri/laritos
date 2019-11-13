#include <log.h>

#include <stdint.h>
#include <cpu.h>
#include <sched/context.h>
#include <mm/exc-handlers.h>
#include <process/pcb.h>
#include <arch/context-types.h>

void exc_undef_handler(int32_t pc, spctx_t *ctx) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT");
    }

    pcb_set_current_pcb_stack_context(ctx);
}

void exc_prefetch_handler(int32_t pc, spctx_t *ctx) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT");
    }

    pcb_set_current_pcb_stack_context(ctx);
}

void exc_abort_handler(int32_t pc, spctx_t *ctx) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT");
    }

    pcb_set_current_pcb_stack_context(ctx);
}
