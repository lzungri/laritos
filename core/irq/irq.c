#include <log.h>

#include <irq.h>
#include <cpu/cpu.h>
#include <core.h>
#include <component/component.h>
#include <component/intc.h>
#include <sched/context.h>
#include <sched/core.h>

int irq_handler(spctx_t *ctx) {
    if (_laritos.process_mode) {
        process_set_current_pcb_stack_context(ctx);
    }

    int fret = 0;
    component_t *c = NULL;
    for_each_component_type(c, COMP_TYPE_INTC) {
        insane_async("Dispatching irq to int controller '%s'", c->id);
        intc_t *intc = (intc_t *) c;
        irqret_t ret = intc->ops.dispatch_irq(intc);
        insane_async("Interrupt controller '%s' returned %s", c->id, irq_get_irqret_str(ret));
        switch (ret) {
        case IRQ_RET_HANDLED:
            goto end;
        case IRQ_RET_ERROR:
            error_async("Error while dispatching irq with intc '%s'", c->id);
            fret = -1;
            goto end;
        case IRQ_RET_NOT_HANDLED:
        case IRQ_RET_HANDLED_KEEP_PROCESSING:
        default:
            break;
        }
    }

end:
    schedule_if_needed();
    return fret;
}
