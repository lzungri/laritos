#pragma once

#include <stdbool.h>
#include <arch/cpu.h>

typedef regpsr_t irqctx_t;

static inline bool arch_irq_is_enabled(void) {
    return !arch_cpu_get_cpsr().b.irq;
}

static inline bool arch_irq_is_enabled_in_ctx(irqctx_t *ctx) {
    return !ctx->b.irq;
}

static inline int arch_irq_disable_local(void) {
    asm("cpsid i");
    return 0;
}

static inline int arch_irq_enable_local(void) {
    asm("cpsie i");
    return 0;
}

static inline int arch_irq_save_context(irqctx_t *ctx) {
    asm("mrs %0, cpsr" : "=r" (*ctx));
    return 0;
}

static inline int arch_irq_disable_local_and_save_ctx(irqctx_t *ctx) {
    arch_irq_save_context(ctx);
    return arch_irq_disable_local();
}

static inline int arch_irq_local_restore_ctx(irqctx_t *ctx) {
    // Note that cpsr_c (control flags) is used instead of cpsr in
    // the msr instruction. This will just copy the control flags instead of
    // the control + condition code flags
    asm("msr cpsr_c, %0" : : "r" (*ctx));
    return 0;
}
