#include <log.h>

#include <stdbool.h>
#include <board.h>
#include <utils/function.h>
#include <component/cpu.h>

DEF_NOT_IMPL_FUNC(ni_set_irqs_enable, cpu_t *c, bool enabled);

int cpu_component_init(cpu_t *c, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) c, bcomp->id, bcomp, COMP_TYPE_CPU, init, deinit) < 0) {
        error("Failed to initialize '%s' cpu component", bcomp->id);
        return -1;
    }

    c->ops.set_irqs_enable = ni_set_irqs_enable;

    if (board_get_int_attr(bcomp, "id", (int *) &c->id) < 0) {
        error("invalid or no cpu id specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "intc", (component_t **) &c->intc) < 0) {
        error("invalid or no interrupt controller specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "sched", (component_t **) &c->sched) < 0) {
        error("invalid or no scheduler specified in the board info");
        return -1;
    }

    if (board_get_u64_attr(bcomp, "freq", &c->freq) < 0) {
        error("invalid or no cpu frequency specified in the board info");
        return -1;
    }

    return 0;
}
