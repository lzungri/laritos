#include <log.h>

#include <board/types.h>
#include <cpu/cpu.h>
#include <dstruct/list.h>
#include <component/component.h>
#include <component/intc.h>
#include <utils/utils.h>
#include <utils/function.h>
#include <mm/heap.h>
#include <sync/atomic.h>
#include <generated/autoconf.h>
#include <irq/types.h>

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
    insane_async("Handling irq %u with int controller '%s'", irq, ((component_t *) intc)->id);

    // Update IRQs stats
    atomic32_inc(&_laritos.stats.nirqs[irq]);

    irqret_t ret = IRQ_RET_NOT_HANDLED;

    irq_handler_info_t *hi;
    list_for_each_entry(hi, &intc->handlers[irq], list) {
        if (hi->h != NULL) {
            ret = hi->h(irq, hi->data);
            insane_async("irq %u processed with handler 0x%p(data=0x%p) = %s", irq, hi->h, hi->data, irq_get_irqret_str(ret));
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

    // Make sure the handler is not already there
    irq_handler_info_t *hi;
    list_for_each_entry(hi, &intc->handlers[irq], list) {
        if (hi->h == h) {
            verbose("Handler 0x%p(data=0x%p) for irq %u already setup, ignoring...", h, data, irq);
            return 0;
        }
    }

    hi = calloc(1, sizeof(irq_handler_info_t));
    if (hi == NULL) {
        error("Couldn't allocate memory for irq_handler_info_t");
        return -1;
    }
    hi->h = h;
    hi->data = data;
    INIT_LIST_HEAD(&hi->list);
    list_add_tail(&hi->list, &intc->handlers[irq]);
    return 0;
}

static int remove_irq_handler(intc_t *intc, irq_t irq, irq_handler_t h) {
    verbose("Removing handler 0x%p for irq %u", h, irq);
    if (irq > CONFIG_INT_MAX_IRQS) {
        error("Invalid irq %u, max_supported: %u", irq, CONFIG_INT_MAX_IRQS);
        return -1;
    }
    irq_handler_info_t *hi;
    irq_handler_info_t *hitemp;
    list_for_each_entry_safe(hi, hitemp, &intc->handlers[irq], list) {
        if (hi->h == h) {
            list_del(&hi->list);
            free(hi);
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

int intc_component_init(intc_t *intc, char *id, board_comp_t *bcomp,
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

    int i;
    for (i = 0; i < ARRAYSIZE(intc->handlers); i++) {
        INIT_LIST_HEAD(&intc->handlers[i]);
    }
    return 0;
}
