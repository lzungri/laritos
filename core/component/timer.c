#include <log.h>
#include <board.h>
#include <component/component.h>
#include <component/timer.h>
#include <utils/function.h>

int timer_init(timer_comp_t *t) {
    // Setup irq stuff if using interrupt-driven io
    if (t->intio) {
        if (intc_enable_irq_with_handler(t->intc,
                t->irq, t->irq_trigger, t->irq_handler, t) < 0) {
            error("Failed to enable irq %u with handler 0x%p", t->irq, t->irq_handler);
            return -1;
        }
    }
    return 0;
}

int timer_deinit(timer_comp_t *t) {
    if (t->intio) {
        return intc_disable_irq_with_handler(t->intc, t->irq, t->irq_handler);
    }
    return 0;
}


DEF_NOT_IMPL_FUNC(ni_get_value, timer_comp_t *t, uint64_t *v);
DEF_NOT_IMPL_FUNC(ni_set_value, timer_comp_t *t, uint64_t v);
DEF_NOT_IMPL_FUNC(ni_get_remaining, timer_comp_t *t, int64_t *v);
DEF_NOT_IMPL_FUNC(ni_reset, timer_comp_t *t);
DEF_NOT_IMPL_FUNC(ni_set_enable, timer_comp_t *t, bool enable);
DEF_NOT_IMPL_FUNC(ni_set_expiration, timer_comp_t *t, int64_t secs, int32_t ns, timer_exp_type_t type);

int timer_component_init(timer_comp_t *t, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, type, init, deinit) < 0) {
        error("Failed to initialize '%s' timer component", bcomp->id);
        return -1;
    }

    t->ops.get_value = ni_get_value;
    t->ops.set_value = ni_set_value;
    t->ops.get_remaining = ni_get_remaining;
    t->ops.reset = ni_reset;
    t->ops.set_enable = ni_set_enable;
    t->ops.set_expiration = ni_set_expiration;

    board_get_bool_attr_def(bcomp, "intio", &t->intio, false);
    if (t->intio) {
        int irq;
        if (board_get_int_attr(bcomp, "irq", &irq) < 0 || irq < 0) {
            error("Invalid or no irq was specified in the board info");
            return -1;
        }
        t->irq = irq;
        board_get_irq_trigger_attr_def(bcomp, "trigger", &t->irq_trigger, IRQ_TRIGGER_LEVEL_HIGH);

        if (board_get_component_attr(bcomp, "intc", (component_t **) &t->intc) < 0) {
            error("invalid or no interrupt controller specified in the board info");
            return -1;
        }
    }

    board_get_long_attr_def(bcomp, "res", (long *) &t->resolution_ns, 0);

    return 0;
}