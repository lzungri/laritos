#include <log.h>

#include <irq.h>
#include <board-types.h>
#include <cpu.h>
#include <component/component.h>
#include <component/intc.h>
#include <utils/utils.h>
#include <utils/function.h>
#include <generated/autoconf.h>

int intc_enable_irq_with_handler(intc_t *intc, irq_t irq, irq_trigger_mode_t tmode, irq_handler_t h, void *data) {
    if (intc->ops.set_irq_trigger_mode(intc, irq, tmode) < 0) {
        error("Failed to set trigger mode for irq %u", irq);
        goto error_irq_enable;
    }
    if (intc->ops.set_irq_enable(intc, irq, true)) {
        error("Couldn't enable irq %u", irq);
        goto error_irq_enable;
    }
    if (intc->ops.set_irq_target_cpus(intc, irq, BIT_FOR_CPU(cpu_get_id())) < 0) {
        error("Failed to set the irq targets");
        goto error_target;
    }
    if (intc->ops.add_irq_handler(intc, irq, h, data) < 0) {
        error("Failed to add handler 0x%p for irq %u", h, irq);
        goto error_handler;
    }

    return 0;

error_handler:
    intc->ops.set_irq_target_cpus(intc, irq, 0);
error_target:
    intc->ops.set_irq_enable(intc, irq, false);
error_irq_enable:
    return -1;
}

static irqret_t handle_irq(intc_t *intc, irq_t irq) {
    verbose_async("Handling irq %u with int controller '%s'", irq, ((component_t *) intc)->id);

    irqret_t ret = IRQ_RET_NOT_HANDLED;
    int i;
    for (i = 0; i < ARRAYSIZE(intc->handlers[0]); i++) {
        irq_handler_info_t *hi = &intc->handlers[irq][i];
        if (hi->h != NULL) {
            ret = hi->h(irq, hi->data);
            verbose_async("irq %u processed with handler 0x%p(data=0x%p) = %s", irq, hi->h, hi->data, get_irqret_str(ret));
            switch (ret) {
            case IRQ_RET_ERROR:
                error_async("Failed to process irq %u with handler 0x%p(data=0x%p)", irq, hi->h, hi->data);
            case IRQ_RET_HANDLED:
                return ret;
            default:
                // Keep processing
                break;
            }
        }
    }
    return ret;
}

/**
 * Add an irq handler to process <irq>
 *
 * TODO: We should be able to specify the priority (order) of the handler
 */
static int add_irq_handler(intc_t *intc, irq_t irq, irq_handler_t h, void *data) {
    verbose("Adding handler 0x%p(data=0x%p) for irq %u", h, data, irq);

    if (irq > CONFIG_INT_MAX_IRQS) {
        error("Invalid irq %u, max_supported: %u", irq, CONFIG_INT_MAX_IRQS);
        return -1;
    }

    if (h == NULL) {
        error("Handler for irq %u cannot be NULL", irq);
        return -1;
    }

    int i;
    for (i = 0; i < ARRAYSIZE(intc->handlers[0]); i++) {
        irq_handler_info_t *hi = &intc->handlers[irq][i];
        if (hi->h == NULL || hi->h == h) {
            hi->h = h;
            hi->data = data;
            return 0;
        }
    }
    error("Couldn't add handler 0x%p. Max number of handlers reached (%u) for irq %u", h, i, irq);
    return -1;
}

static int remove_irq_handler(intc_t *intc, irq_t irq, irq_handler_t h) {
    verbose("Removing handler 0x%p for irq %u", h, irq);
    if (irq > CONFIG_INT_MAX_IRQS) {
        error("Invalid irq %u, max_supported: %u", irq, CONFIG_INT_MAX_IRQS);
        return -1;
    }
    int i;
    for (i = 0; i < ARRAYSIZE(intc->handlers[0]); i++) {
        irq_handler_info_t *hi = &intc->handlers[irq][i];
        if (hi->h == h) {
            hi->h = NULL;
            hi->data = NULL;
            return 0;
        }
    }
    return 0;
}

DEF_NOT_IMPL_FUNC(ni_set_irq_enable, intc_t *intc, irq_t irq, bool enabled);
DEF_NOT_IMPL_FUNC(ni_set_irq_trigger_mode, intc_t *intc, irq_t irq, irq_trigger_mode_t mode);
DEF_NOT_IMPL_FUNC(ni_set_irq_target_cpus, intc_t *intc, irq_t irq, cpubits_t bits);
DEF_NOT_IMPL_FUNC(ni_set_irqs_enable_for_this_cpu, intc_t *intc, bool enabled);
DEF_NOT_IMPL_FUNC(ni_set_priority_filter, intc_t *intc, uint8_t lowest_prio);

int intc_init(intc_t *intc, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    if (component_init((component_t *) intc, id, bcomp, COMP_TYPE_INTC, init, deinit) < 0) {
        error("Couldn't initialize intc '%s' component", id);
        return -1;
    }

    intc->ops.handle_irq = handle_irq;
    intc->ops.set_irq_enable = ni_set_irq_enable;
    intc->ops.set_irq_trigger_mode = ni_set_irq_trigger_mode;
    intc->ops.set_irq_target_cpus = ni_set_irq_target_cpus;
    intc->ops.set_irqs_enable_for_this_cpu = ni_set_irqs_enable_for_this_cpu;
    intc->ops.set_priority_filter = ni_set_priority_filter;
    intc->ops.add_irq_handler = add_irq_handler;
    intc->ops.remove_irq_handler = remove_irq_handler;
    return 0;
}
