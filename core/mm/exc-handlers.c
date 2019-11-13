#include <log.h>

#include <stdint.h>
#include <mm/exc-handlers.h>
#include <arch/context-types.h>
#include <process/pcb.h>

void exc_undef_handler(int32_t pc, spctx_t *ctx) {
    pcb_set_current_pcb_stack_context(ctx);



    fatal("ABORT");
}

void exc_prefetch_handler(int32_t pc, spctx_t *ctx) {
    pcb_set_current_pcb_stack_context(ctx);

    fatal("ABORT");
}

void exc_abort_handler(int32_t pc, spctx_t *ctx) {
    pcb_set_current_pcb_stack_context(ctx);

    fatal("ABORT");
}
