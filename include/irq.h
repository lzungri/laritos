#pragma once

#include <stdint.h>
#include <utils/utils.h>
#include <arch/irq.h>
#include <arch/stack.h>


typedef enum {
    IRQ_TRIGGER_EDGE_LOW_HIGH = 1,
    IRQ_TRIGGER_EDGE_HIGH_LOW = 2,
    IRQ_TRIGGER_LEVEL_HIGH = 4,
    IRQ_TRIGGER_LEVEL_LOW = 8,
} irq_trigger_mode_t;

typedef enum {
    IRQ_RET_ERROR = -1,
    IRQ_RET_HANDLED,
    IRQ_RET_HANDLED_KEEP_PROCESSING,
    IRQ_RET_NOT_HANDLED,

    IRQ_RET_LEN,
} irqret_t;

typedef uint16_t irq_t;

typedef irqret_t (*irq_handler_t)(irq_t irq, void *data);

/**
 * Main function to dispatch and process irqs
 *
 * @param ctx: Context saved by the irq handler
 * @return 0 on success, <0 on error
 */
int irq_handler(const spctx_t *ctx);

/**
 * @return: IRQ return value string for the given <value>
 */
static inline const char *get_irqret_str(irqret_t ret) {
    static const char *str[IRQ_RET_LEN + 1] = {
        "IRQ_RET_ERROR", "IRQ_RET_HANDLED", "IRQ_RET_HANDLED_KEEP_PROCESSING", "IRQ_RET_NOT_HANDLED",
    };
    ret += 1;
    return ret < ARRAYSIZE(str) && ret >= 0 && str[ret] != NULL ? str[ret] : "???";
}

static inline int disable_local_irq(void) {
    return arch_disable_local_irq();
}

static inline int enable_local_irq(void) {
    return arch_enable_local_irq();
}

static inline int disable_local_irq_save_ctx(irqctx_t *ctx) {
    return arch_disable_local_irq_save_ctx(ctx);
}

static inline int local_irq_restore_ctx(irqctx_t *ctx) {
    return arch_local_irq_restore_ctx(ctx);
}
