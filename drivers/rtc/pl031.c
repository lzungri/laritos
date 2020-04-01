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
#include <board/types.h>
#include <board/core.h>
#include <time/core.h>
#include <driver/core.h>
#include <driver/pl031.h>
#include <utils/utils.h>
#include <utils/math.h>
#include <mm/heap.h>
#include <component/timer.h>
#include <component/component.h>


static int get_value(timer_comp_t *t, uint64_t *v) {
    rtc_t *rtc = (rtc_t *) t;
    *v = rtc->mm->data;
    return 0;
}

static int set_value(timer_comp_t *t, uint64_t v) {
    irqctx_t ctx;
    spinlock_acquire(&t->lock, &ctx);

    rtc_t *rtc = (rtc_t *) t;
    rtc->mm->load = (uint32_t) v;

    spinlock_release(&t->lock, &ctx);
    return 0;
}

static int get_remaining(timer_comp_t *t, int64_t *v) {
    rtc_t *rtc = (rtc_t *) t;
    *v = rtc->mm->match - rtc->mm->data;
    return 0;
}

static int reset(timer_comp_t *t) {
    irqctx_t ctx;
    spinlock_acquire(&t->lock, &ctx);

    rtc_t *rtc = (rtc_t *) t;
    // According to the pl031 doc, writing 1 to ctrl reg will reset the rtc value,
    // but qemu just ignores this write operation
    rtc->mm->control = 1;
    rtc->mm->load = 0;
    rtc->mm->int_clear = 1;
    rtc->mm->int_mask = 0;

    spinlock_release(&t->lock, &ctx);
    return 0;
}

static int set_enable_locked(timer_comp_t *t, bool enable) {
    rtc_t *rtc = (rtc_t *) t;
    rtc->mm->control = enable ? 1 : 0;
    rtc->mm->int_mask = enable ? 1 : 0;
    return 0;
}

static int set_enable(timer_comp_t *t, bool enable) {
    irqctx_t ctx;
    spinlock_acquire(&t->lock, &ctx);

    set_enable_locked(t, enable);

    spinlock_release(&t->lock, &ctx);
    return 0;
}

static int set_expiration_ticks(timer_comp_t *t, int64_t timer_ticks, timer_exp_type_t type,
        timer_cb_t cb, void *data, bool periodic) {
    irqctx_t ctx;
    spinlock_acquire(&t->lock, &ctx);

    // Disable the timer. We may not need to do this since we are already disabling
    // irqs, but just in case
    set_enable_locked(t, false);

    rtc_t *rtc = (rtc_t *) t;

    switch (type) {
    case TIMER_EXP_ABSOLUTE:
        verbose_async("set_expiration_ticks(pl031, timer_ticks=%lu, TIMER_EXP_ABSOLUTE)", (uint32_t) timer_ticks);
        rtc->mm->match = max(timer_ticks, 0);
        break;
    case TIMER_EXP_RELATIVE:
        verbose_async("set_expiration_ticks(pl031, timer_ticks=%lu, TIMER_EXP_RELATIVE)", (uint32_t) timer_ticks);
        rtc->mm->match = max((int64_t) rtc->mm->data + timer_ticks, 0);
        break;
    }

    t->curtimer.cb = cb;
    t->curtimer.data = data;
    t->curtimer.ticks = timer_ticks;
    t->curtimer.periodic = periodic;
    t->curtimer.enabled = true;

    // Re-enable the timer
    set_enable_locked(t, true);

    spinlock_release(&t->lock, &ctx);

    return 0;
}

static int clear_expiration(timer_comp_t *t) {
    irqctx_t ctx;
    spinlock_acquire(&t->lock, &ctx);

    rtc_t *rtc = (rtc_t *) t;
    rtc->mm->match = 0;
    t->curtimer.enabled = false;

    spinlock_release(&t->lock, &ctx);
    return 0;
}

static irqret_t pl031_irq_handler(irq_t irq, void *data) {
    verbose_async("rtc '%s' expired", ((component_t *) data)->id);

    rtc_t *rtc = (rtc_t *) data;
    // Clear interrupt
    rtc->mm->int_clear = 1;

    return timer_handle_expiration((timer_comp_t *) rtc);
}

static int init(component_t *c) {
    timer_comp_t *t = (timer_comp_t *) c;

    if (timer_init(t) < 0) {
        error("Failed to initialize timer for component '%s'", c->id);
        return -1;
    }

    rtc_t *rtc = (rtc_t *) c;
    // Enable RTC interrupts
    rtc->mm->int_mask = 1;
    return 0;
}

static int deinit(component_t *c) {
    timer_comp_t *t = (timer_comp_t *) c;
    return timer_deinit(t);
}

static int process(board_comp_t *comp) {
    rtc_t *rtc = component_alloc(sizeof(rtc_t));
    if (rtc == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }
    timer_comp_t *t = (timer_comp_t *) rtc;

    if (timer_component_init(t, comp, COMP_TYPE_RTC, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }
    t->irq_handler = pl031_irq_handler;
    t->ops.get_value = get_value;
    t->ops.set_value = set_value;
    t->ops.get_remaining = get_remaining;
    t->ops.reset = reset;
    t->ops.set_enable = set_enable;
    t->ops.set_expiration_ticks = set_expiration_ticks;
    t->ops.clear_expiration = clear_expiration;

    board_get_ptr_attr_def(comp, "mmbase", (void **) &rtc->mm, NULL);
    if (rtc->mm == NULL) {
        error("No memory-mapped regs base was specified in the board information");
        goto fail;
    }

    component_set_info((component_t *) rtc, "PrimeCell RTC (pl031)", "ARM", "RTC AMBA compliant SoC");

    if (timer_component_register(t) < 0) {
        error("Couldn't register rtc '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(rtc);
    return -1;
}

DRIVER_MODULE(pl031, process);
