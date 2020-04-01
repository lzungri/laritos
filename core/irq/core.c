/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <irq/types.h>
#include <cpu/core.h>
#include <core.h>
#include <component/component.h>
#include <component/intc.h>
#include <sched/context.h>
#include <sched/core.h>
#include <generated/autoconf.h>

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



#ifdef CONFIG_TEST_CORE_IRQ
#include __FILE__
#endif
