#pragma once

#include <stdbool.h>
#include <component.h>
#include <stdint.h>
#include <cpu.h>
#include <irq.h>
#include <component.h>
#include <hwcomp.h>
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
} irq_handler_info_t;

typedef struct intc{
    hwcomp_t parent;
    intc_ops_t ops;

    irq_handler_info_t handlers[CONFIG_MAX_IRQS][CONFIG_MAX_HANDLERS_PER_IRQ];
} intc_t;


/**
 * Helper function to enable an irq and associate it with a handler
 */
static inline int intc_enable_irq_with_handler(intc_t *intc, irq_t irq, irq_trigger_mode_t tmode, irq_handler_t h, void *data) {
    if (intc->ops.set_irq_trigger_mode(intc, irq, tmode) < 0) {
        error("Failed to set trigger mode for irq %u", irq);
        goto error_irq_enable;
    }
    if (intc->ops.set_irq_enable(intc, irq, true)) {
        error("Couldn't enable irq %u", irq);
        goto error_irq_enable;
    }
    if (intc->ops.set_irq_target_cpus(intc, irq, BIT_FOR_CPU(get_cpu_id())) < 0) {
        error("Failed to set the irq targets");
        goto error_target;
    }
    if (intc->ops.set_irqs_enable_for_this_cpu(intc, true) < 0) {
        error("Failed to enable irqs for this cpu");
        goto error_cpu_enable;
    }
    if (intc->ops.add_irq_handler(intc, irq, h, data) < 0) {
        error("Failed to add handler 0x%p for irq %u", h, irq);
        goto error_handler;
    }

    return 0;

error_handler:
    intc->ops.set_irqs_enable_for_this_cpu(intc, false);
error_cpu_enable:
    intc->ops.set_irq_target_cpus(intc, irq, 0);
error_target:
    intc->ops.set_irq_enable(intc, irq, false);
error_irq_enable:
    return -1;
}

/**
 * Helper function to disable an irq and remove its handler
 */
static inline int intc_disable_irq_with_handler(intc_t *intc, irq_t irq, irq_handler_t h) {
    intc->ops.remove_irq_handler(intc, irq, h);
    intc->ops.set_irqs_enable_for_this_cpu(intc, false);
    intc->ops.set_irq_target_cpus(intc, irq, 0);
    return intc->ops.set_irq_enable(intc, irq, false);
}

int intc_init(intc_t *intc, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
