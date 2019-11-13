#include <log.h>

#include <stdint.h>
#include <cpu.h>
#include <sched/context.h>
#include <mm/exc-handlers.h>
#include <process/pcb.h>
#include <process/status.h>
#include <arch/context-types.h>
#include <utils/debug.h>
#include <utils/math.h>

static void dump_process_info(pcb_t *pcb) {
    error_async("pid=%u, context=0x%p, status=%s",
            pcb->pid, pcb->mm.sp_ctx, pcb_get_status_str(pcb->sched.status));
    error_async("ABORT");
    message_delimiter();
}

void exc_undef_handler(int32_t pc, spctx_t *ctx) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT");
    }

    pcb_set_current_pcb_stack_context(ctx);

    pcb_t *pcb = pcb_get_current();
    dump_process_info(pcb);
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
