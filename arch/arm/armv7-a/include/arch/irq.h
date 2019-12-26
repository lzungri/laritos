#pragma once

#include <arch/cpu.h>

typedef regpsr_t irqctx_t;

static inline int arch_disable_local_irq(void) {
    asm("cpsid i");
    return 0;
}

static inline int arch_enable_local_irq(void) {
    asm("cpsie i");
    return 0;
}

static inline int arch_disable_local_irq_save_ctx(irqctx_t *ctx) {
    // TODO Prevent the race condition between get_cpsr() and disable irq
    *ctx = arch_get_cpsr();
    return arch_disable_local_irq();
}

static inline int arch_local_irq_restore_ctx(irqctx_t *ctx) {
    arch_set_cpsr(ctx);
    return 0;
}
