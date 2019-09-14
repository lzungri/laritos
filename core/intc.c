#define DEBUG
#include <log.h>

#include <board.h>
#include <component.h>
#include <intc.h>
#include <irq.h>

#define NOT_IMPL_FUNC(_func, ...) \
    static int _func(__VA_ARGS__) { \
        error("Not Implemented"); \
        return -1; \
    }

NOT_IMPL_FUNC(ni_enable_irq, intc_t *intc, irq_t irq);
NOT_IMPL_FUNC(ni_disable_irq, intc_t *intc, irq_t irq);
NOT_IMPL_FUNC(ni_set_irq_trigger_mode, intc_t *intc, irq_t irq, irq_trigger_mode_t mode);
NOT_IMPL_FUNC(ni_set_irq_target_cpus, intc_t *intc, irq_t irq, cpubits_t bits);
NOT_IMPL_FUNC(enable_int_signaling_to_this_cpu, intc_t *intc);
NOT_IMPL_FUNC(ni_set_priority_filter, intc_t *intc, uint8_t lowest_prio);

static irqret_t handle_irq(intc_t *intc, irq_t irq) {
    verbose("Handling irq %u with int controller '%s'", irq, intc->parent.id);
    return IRQ_RET_HANDLED;
}

static int add_irq_handler(intc_t *intc, irq_t irq, irq_handler_t h, component_t *comp) {
    return -1;
}

static int remove_irq_handler(intc_t *intc, irq_t irq, irq_handler_t h) {
    return -1;
}

int intc_init(intc_t *comp, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    if (component_init((component_t *) comp, id, bcomp, COMP_TYPE_INTC, init, deinit) < 0) {
        error("Couldn't initialize intc '%s' component", id);
        return -1;
    }

    comp->ops.handle_irq = handle_irq;
    comp->ops.enable_irq = ni_enable_irq;
    comp->ops.disable_irq = ni_disable_irq;
    comp->ops.set_irq_trigger_mode = ni_set_irq_trigger_mode;
    comp->ops.set_irq_target_cpus = ni_set_irq_target_cpus;
    comp->ops.enable_int_signaling_to_this_cpu = enable_int_signaling_to_this_cpu;
    comp->ops.set_priority_filter = ni_set_priority_filter;
    comp->ops.add_irq_handler = add_irq_handler;
    comp->ops.remove_irq_handler = remove_irq_handler;
    return 0;
}
