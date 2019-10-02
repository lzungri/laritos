#include <log.h>
#include <irq.h>
#include <board-types.h>
#include <board.h>
#include <driver/driver.h>
#include <driver/pl031.h>
#include <utils/utils.h>
#include <component/timer.h>
#include <component/component.h>

#define MAX_RTCS 3

// TODO Use dynamic memory instead
static rtc_t rtcs[MAX_RTCS];
static uint8_t cur_rtc;

static irqret_t irq_handler(irq_t irq, void *data) {
    rtc_t *rtc = (rtc_t *) data;

    // Clear interrupt
    rtc->mm->int_clear = 1;
    rtc->mm->match = rtc->mm->data + 5;
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
    rtc->mm->match = rtc->mm->data + 5;
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

    t->irq_handler = irq_handler;
    if (timer_component_init(t, comp, COMP_TYPE_RTC, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        return -1;
    }

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
