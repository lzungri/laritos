#pragma once

#include <stdbool.h>
#include <arch/cpu.h>

typedef regpsr_t irqctx_t;

static inline bool arch_irq_is_enabled(void) {
    return !arch_get_cpsr().b.irq;
}

static inline int arch_irq_disable_local(void) {
    asm("cpsid i");
    return 0;
}

static inline int arch_irq_enable_local(void) {
    asm("cpsie i");
    return 0;
}

static inline int arch_irq_disable_local_and_save_ctx(irqctx_t *ctx) {
    *ctx = arch_get_cpsr();
    return arch_irq_disable_local();
}

static inline int arch_irq_local_restore_ctx(irqctx_t *ctx) {
    arch_set_cpsr(ctx);
    return 0;
}
