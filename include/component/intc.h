#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <cpu.h>
#include <irq.h>
#include <dstruct/list.h>
#include <component/component.h>
#include <generated/autoconf.h>

struct intc;
typedef struct {
    irqret_t (*dispatch_irq)(struct intc *intc);
    irqret_t (*handle_irq)(struct intc *intc, irq_t irq);
    int (*add_irq_handler)(struct intc *intc, irq_t irq, irq_handler_t h, void *data);
    int (*remove_irq_handler)(struct intc *intc, irq_t irq, irq_handler_t h);

    int (*set_irq_enable)(struct intc *intc, irq_t irq, bool enabled);
    int (*set_irq_trigger_mode)(struct intc *intc, irq_t irq, irq_trigger_mode_t mode);
    int (*set_irq_target_cpus)(struct intc *intc, irq_t irq, cpubits_t bits);
    int (*set_irqs_enable_for_this_cpu)(struct intc *intc, bool enabled);
    /**
     * Provides an interrupt priority filter. Only interrupts with higher priority
     * than this value are signaled to the processor
     *
     * Note: Higher priority corresponds to a lower Priority field value
     */
    int (*set_priority_filter)(struct intc *intc, uint8_t lowest_prio);
} intc_ops_t;

typedef struct {
    irq_handler_t h;
    void *data;

    struct list_head list;
} irq_handler_info_t;

typedef struct intc{
    component_t parent;
    intc_ops_t ops;

    struct list_head handlers[CONFIG_INT_MAX_IRQS];
} intc_t;


/**
 * Helper function to disable an irq and remove its handler
 */
static inline int intc_disable_irq_with_handler(intc_t *intc, irq_t irq, irq_handler_t h) {
    intc->ops.remove_irq_handler(intc, irq, h);
    intc->ops.set_irq_target_cpus(intc, irq, 0);
    return intc->ops.set_irq_enable(intc, irq, false);
}

/**
 * Helper function to enable an irq and associate it with a handler
 */
int intc_enable_irq_with_handler(intc_t *intc, irq_t irq, irq_trigger_mode_t tmode, irq_handler_t h, void *data);
int intc_component_init(intc_t *intc, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
