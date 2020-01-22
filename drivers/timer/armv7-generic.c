//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <irq.h>
#include <board/board-types.h>
#include <board/board.h>
#include <time/time.h>
#include <driver/driver.h>
#include <driver/armv7-generic-timer.h>
#include <arch/generic-timer.h>
#include <utils/utils.h>
#include <utils/math.h>
#include <mm/heap.h>
#include <component/timer.h>
#include <component/component.h>


static int get_value(timer_comp_t *t, uint64_t *v) {
    *v = arch_get_cntp_ct();
    return 0;
}

static int get_remaining(timer_comp_t *t, int64_t *v) {
    *v = arch_get_cntp_cval() - arch_get_cntp_ct();
    return 0;
}

static int set_enable_locked(timer_comp_t *t, bool enable) {
    regcntpctl_t ctl = { 0 };
    ctl.b.enable = enable;
    arch_set_cntp_ctl(ctl);
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

    switch (type) {
    case TIMER_EXP_ABSOLUTE:
        insane_async("setexpticks(armv7, ticks=%lu, TIMER_EXP_ABSOLUTE)", (uint32_t) timer_ticks);
        arch_set_cntp_cval(max(timer_ticks, 0));
        break;
    case TIMER_EXP_RELATIVE:
        insane_async("setexpticks(armv7, ticks=%lu, TIMER_EXP_RELATIVE)", (uint32_t) timer_ticks);
        arch_set_cntp_cval(max((int64_t) arch_get_cntp_cval() + timer_ticks, 0));
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

    set_enable_locked(t, false);
    arch_set_cntp_cval(0);

    spinlock_release(&t->lock, &ctx);
    return 0;
}

static irqret_t armv7_timer_irq_handler(irq_t irq, void *data) {
    insane_async("armv7-timer '%s' expired", ((component_t *) data)->id);

    timer_comp_t *t = (timer_comp_t *) data;
    // Disable future irqs coming from the same compare value
    set_enable(t, false);

    return timer_handle_expiration(t);
}

static int init(component_t *c) {
    timer_comp_t *t = (timer_comp_t *) c;

    if (timer_init(t) < 0) {
        error("Failed to initialize timer for component '%s'", c->id);
        return -1;
    }

    /**
     * According to ARM ARM:
     *   The CNTFRQ register is UNKNOWN at reset, and therefore the counter
     *   frequency must be written to CNTFRQ as part of the system boot process
     */
    arch_set_cntfrq(t->maxfreq);
    // For now, do not signal interrupts based on the CNTP_CVAL register
    set_enable(t, false);
    // Reset compare value
    arch_set_cntp_cval(0);
    return 0;
}

static int deinit(component_t *c) {
    timer_comp_t *t = (timer_comp_t *) c;
    return timer_deinit(t);
}

static int process(board_comp_t *comp) {
    armv7_timer_t *timer = component_alloc(sizeof(armv7_timer_t));
    if (timer == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }
    timer_comp_t *t = (timer_comp_t *) timer;

    if (timer_component_init(t, comp, COMP_TYPE_HRTIMER, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }
    t->irq_handler = armv7_timer_irq_handler;
    t->ops.get_value = get_value;
    t->ops.get_remaining = get_remaining;
    t->ops.set_enable = set_enable;
    t->ops.set_expiration_ticks = set_expiration_ticks;
    t->ops.clear_expiration = clear_expiration;

    component_set_info((component_t *) timer, "ARM v7 timer", "ARM", "ARM v7 Generic Timer");

    if (component_register((component_t *) timer) < 0) {
        error("Couldn't register '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(timer);
    return -1;
}

DEF_DRIVER_MANAGER(armv7_generic_timer, process);
