#pragma once

#include <stdbool.h>
#include <component.h>
#include <stdint.h>
#include <cpu.h>
#include <irq.h>
#include <component.h>

struct intc;
typedef struct {
    irqret_t (*dispatch_irq)(struct intc *intc);
    irqret_t (*handle_irq)(struct intc *intc, irq_t irq);
    int (*add_irq_handler)(struct intc *intc, irq_t irq, irq_handler_t h, component_t *comp);
    int (*remove_irq_handler)(struct intc *intc, irq_t irq, irq_handler_t h);

    int (*set_irq_enable)(struct intc *intc, irq_t irq, bool enabled);
    int (*set_irq_trigger_mode)(struct intc *intc, irq_t irq, irq_trigger_mode_t mode);
    int (*set_irq_target_cpus)(struct intc *intc, irq_t irq, cpubits_t bits);
    int (*set_irq_signaling_cpu_enable)(struct intc *intc, bool enabled);
    /**
     * Provides an interrupt priority filter. Only interrupts with higher priority
     * than this value are signaled to the processor
     *
     * Note: Higher priority corresponds to a lower Priority field value
     */
    int (*set_priority_filter)(struct intc *intc, uint8_t lowest_prio);
} intc_ops_t;

typedef struct intc{
    component_t parent;
    intc_ops_t ops;
} intc_t;

int intc_init(intc_t *comp, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
