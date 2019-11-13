#include <log.h>

#include <irq.h>
#include <cpu.h>
#include <component/component.h>
#include <component/intc.h>

int irq_handler(const spctx_t *ctx) {
    component_t *c = NULL;
    for_each_component_type(c, COMP_TYPE_INTC) {
        verbose_async("Dispatching irq to int controller '%s'", c->id);
        intc_t *intc = (intc_t *) c;
        irqret_t ret = intc->ops.dispatch_irq(intc);
        verbose_async("Interrupt controller '%s' returned %s", c->id, get_irqret_str(ret));
        switch (ret) {
        case IRQ_RET_HANDLED:
            return 0;
        case IRQ_RET_ERROR:
            error_async("Error while dispatching irq with intc '%s'", c->id);
            return -1;
        case IRQ_RET_NOT_HANDLED:
        case IRQ_RET_HANDLED_KEEP_PROCESSING:
        default:
            break;
        }
    }
    return 0;
}
