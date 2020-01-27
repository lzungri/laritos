#pragma once

#include <stdint.h>
#include <utils/utils.h>
#include <arch/irq.h>
#include <arch/context-types.h>
#include "types.h"

/**
 * Main function to dispatch and process irqs
 *
 * @param ctx: Context saved by the irq handler
 * @return 0 on success, <0 on error
 */
int irq_handler(spctx_t *ctx);

/**
 * @return: IRQ return value string for the given <value>
 */
static inline const char *irq_get_irqret_str(irqret_t ret) {
    static const char *str[IRQ_RET_LEN + 1] = {
        "IRQ_RET_ERROR", "IRQ_RET_HANDLED", "IRQ_RET_HANDLED_KEEP_PROCESSING", "IRQ_RET_NOT_HANDLED",
    };
    ret += 1;
    return ret < ARRAYSIZE(str) && ret >= 0 && str[ret] != NULL ? str[ret] : "???";
}

static inline bool irq_is_enabled(void) {
    return arch_irq_is_enabled();
}

static inline bool irq_is_enabled_in_ctx(irqctx_t *ctx) {
    return arch_irq_is_enabled_in_ctx(ctx);
}

static inline int irq_save_context(irqctx_t *ctx) {
    return arch_irq_save_context(ctx);
}

static inline int irq_disable_local(void) {
    return arch_irq_disable_local();
}

static inline int irq_enable_local(void) {
    return arch_irq_enable_local();
}

static inline int irq_disable_local_and_save_ctx(irqctx_t *ctx) {
    return arch_irq_disable_local_and_save_ctx(ctx);
}

static inline int irq_local_restore_ctx(irqctx_t *ctx) {
    return arch_irq_local_restore_ctx(ctx);
}
