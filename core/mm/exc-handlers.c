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
    error_async("pid=%u, name=%s, type=%s, context=0x%p, status=%s, psr=%s",
            pcb->pid, pcb->name, pcb->kernel ? "K" : "U",
            pcb->mm.sp_ctx, pcb_get_status_str(pcb->sched.status),
            arch_get_psr_str(arch_get_spsr(), buf, sizeof(buf)));
}

static inline void handle_process_exception(pcb_t *pcb) {
    dump_process_info(pcb);
    debug_message_delimiter();
    error_async("ABORT");
    // Kill the offending process
    process_kill(pcb);
    // Switch to another ready process
    schedule();
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
