#include <log.h>
#include <irq.h>
#include <board-types.h>
#include <board.h>
#include <time/time.h>
#include <driver/driver.h>
#include <driver/pl031.h>
#include <utils/utils.h>
#include <utils/math.h>
#include <component/timer.h>
#include <component/component.h>

#define MAX_RTCS 3

// TODO Use dynamic memory instead
static rtc_t rtcs[MAX_RTCS];
static uint8_t cur_rtc;


static int get_value(timer_comp_t *t, uint64_t *v) {
    rtc_t *rtc = (rtc_t *) t;
    *v = rtc->mm->data;
    return 0;
}

static int set_value(timer_comp_t *t, uint64_t v) {
    rtc_t *rtc = (rtc_t *) t;
    rtc->mm->load = (uint32_t) v;
    return 0;
}

static int get_remaining(timer_comp_t *t, int64_t *v) {
    rtc_t *rtc = (rtc_t *) t;
    *v = rtc->mm->match - rtc->mm->data;
    return 0;
}

static int reset(timer_comp_t *t) {
    rtc_t *rtc = (rtc_t *) t;
    // According to the pl031 doc, writing 1 to ctrl reg will reset the rtc value,
    // but qemu just ignores this write operation
    rtc->mm->control = 1;
    rtc->mm->load = 0;
    rtc->mm->int_clear = 1;
    rtc->mm->int_mask = 0;
    return 0;
}

static int set_enable(timer_comp_t *t, bool enable) {
    rtc_t *rtc = (rtc_t *) t;
    rtc->mm->control = enable ? 1 : 0;
    rtc->mm->int_mask = enable ? 1 : 0;
    return 0;
}

static int set_expiration(timer_comp_t *t, int64_t secs, int32_t ns, timer_exp_type_t type) {
    rtc_t *rtc = (rtc_t *) t;

    // ns must be lower to a second
    if (abs(ns) > NSEC_PER_SEC - 1) {
        ns = (NSEC_PER_SEC - 1) * sign(ns);
    }

    int32_t exp = secs + NS_TO_SEC(ns);
    switch (type) {
    case TIMER_EXP_ABSOLUTE:
        rtc->mm->match = max(exp, 0);
        break;
    case TIMER_EXP_RELATIVE:
        rtc->mm->match = max((int64_t) rtc->mm->data + exp, 0);
        break;
    }

    return 0;
}

static irqret_t irq_handler(irq_t irq, void *data) {
    verbose_async("rtc '%s' expired", ((component_t *) data)->id);

    rtc_t *rtc = (rtc_t *) data;
    // Clear interrupt
    rtc->mm->int_clear = 1;
    return IRQ_RET_HANDLED;
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
    if (cur_rtc > ARRAYSIZE(rtcs)) {
        error("Max number of rtcs components reached");
        return -1;
    }

    rtc_t *rtc = &rtcs[cur_rtc];
    timer_comp_t *t = (timer_comp_t *) rtc;

    if (timer_component_init(t, comp, COMP_TYPE_RTC, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        return -1;
    }
    t->irq_handler = irq_handler;
    t->ops.get_value = get_value;
    t->ops.set_value = set_value;
    t->ops.get_remaining = get_remaining;
    t->ops.reset = reset;
    t->ops.set_enable = set_enable;
    t->ops.set_expiration = set_expiration;

    board_get_ptr_attr_def(comp, "mmbase", (void **) &rtc->mm, NULL);
    if (rtc->mm == NULL) {
        error("No memory-mapped regs base was specified in the board information");
        return -1;
    }

    component_set_info((component_t *) rtc, "PrimeCell RTC (pl031)", "ARM", "RTC AMBA compliant SoC");

    if (component_register((component_t *) rtc) < 0) {
        error("Couldn't register rtc '%s'", comp->id);
        return -1;
    }

    cur_rtc++;

    return 0;
}

DEF_DRIVER_MANAGER(pl031, process);
