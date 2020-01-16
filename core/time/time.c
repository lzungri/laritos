#include <log.h>

#include <cpu.h>
#include <core.h>
#include <limits.h>
#include <refcount.h>
#include <time/time.h>
#include <time/tick.h>
#include <time/timeconv.h>
#include <component/timer.h>
#include <component/vrtimer.h>
#include <component/component.h>
#include <utils/assert.h>
#include <process/core.h>
#include <sched/core.h>
#include <sync/spinlock.h>

static inline vrtimer_comp_t *get_vrtimer(void) {
    vrtimer_comp_t *vrt = (vrtimer_comp_t *) component_first_of_type(COMP_TYPE_VRTIMER);
    assert(vrt != NULL, "Cannot use xsleep() family without a virtual timer");
    return vrt;
}

int time_get_rtc_time(time_t *t) {
    timer_comp_t *rtc = (timer_comp_t *) component_first_of_type(COMP_TYPE_RTC);
    if (rtc == NULL) {
        return -1;
    }
    t->ns = 0;
    return rtc->ops.get_value(rtc, &t->secs);
}

int time_get_ns_rtc_time(time_t *t) {
    time_t temp;
    if (time_get_monotonic_time(&temp) < 0) {
        return -1;
    }
    time_add(&temp, &_laritos.timeinfo.boottime, t);
    return 0;
}

int time_get_rtc_localtime_calendar(calendar_t *c) {
    time_t time = { 0 };
    if (time_get_rtc_time(&time) < 0) {
        return -1;
    }
    return epoch_to_localtime_calendar(time.secs, c);
}

int time_get_monotonic_time(time_t *t) {
    vrtimer_comp_t *vrt = (vrtimer_comp_t *) component_first_of_type(COMP_TYPE_VRTIMER);
    if (vrt == NULL) {
        return -1;
    }

    timer_comp_t *hrt = vrt->hrtimer;
    uint64_t v;
    if (hrt->ops.get_value(hrt, &v) < 0) {
        return -1;
    }

    t->secs = TICK_TO_SEC(hrt, v);
    v -= SEC_TO_TICK(hrt, t->secs);
    t->ns = TICK_TO_NS(hrt, v);
    return 0;
}

int time_set_timezone(timezone_t t, bool daylight) {
    _laritos.timeinfo.tz = t;
    _laritos.timeinfo.dst = daylight;
    return 0;
}

int time_get_localtime_offset(void) {
    int secs = _laritos.timeinfo.tz;
    secs += _laritos.timeinfo.dst ? 1 : 0;
    secs *= SECS_PER_HOUR;
    return secs;
}

void time_to_hms(time_t *t, uint16_t *h, uint16_t *m, uint16_t *s) {
    *h = SEC_TO_HOUR(t->secs);
    *m = SEC_TO_MINUTE(t->secs % SECS_PER_HOUR);
    *s = t->secs % 60;
}

static int process_sleep_cb(vrtimer_comp_t *t, void *data) {
    pcb_t *pcb = (pcb_t *) data;

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proclock, &ctx);

    sched_move_to_ready_locked(pcb);
    // pcb_t no longer needed by sleep()
    ref_dec(&pcb->refcnt);

    spinlock_release(&_laritos.proclock, &ctx);

    return 0;
}

static int non_process_sleep_cb(vrtimer_comp_t *t, void *data) {
    bool *timer_expired = (bool *) data;
    *timer_expired = true;
    return 0;
}

static inline void _sleep(vrtimer_comp_t *t, tick_t ticks) {
    verbose_async("Sleeping for %lu ticks", ticks);

    if (ticks == 0) {
        return;
    }

    if (_laritos.process_mode) {
        pcb_t *pcb = process_get_current();

        irqctx_t ctx;
        spinlock_acquire(&_laritos.proclock, &ctx);

        // Running in process mode, then block the process and schedule()
        if (t->ops.add_vrtimer(t, ticks, process_sleep_cb, pcb, false) < 0) {
            error_async("Failed to create virtual timer ticks=%lu", ticks);
            spinlock_release(&_laritos.proclock, &ctx);
            return;
        }
        // Make sure we install the timer before blocking the process, otherwise
        // the process may block forever if there is a context switch.
        // The spinlock, by disabling local irqs, will ensure no other
        // process preempts the current one until the timer is installed and
        // the process moved to the blocked list
        sched_move_to_blocked_locked(pcb);

        // We are gonna need the pcb_t to unblock it once the timer expires.
        // Make sure it is not released by keeping a reference to it
        ref_inc(&pcb->refcnt);

        spinlock_release(&_laritos.proclock, &ctx);

        schedule();
    } else {
        // Running in non-process mode, then block the kernel thread
        bool timer_expired = false;
        if (t->ops.add_vrtimer(t, ticks, non_process_sleep_cb, &timer_expired, false) < 0) {
            error_async("Failed to create virtual timer ticks=%lu", ticks);
            return;
        }

        // Put the cpu to sleep until the timer expires
        while (!timer_expired) {
            arch_wfi();
        }
    }
}

void sleep(uint32_t secs) {
    vrtimer_comp_t *vrt = get_vrtimer();
    _sleep(vrt, SEC_TO_TICK(vrt->hrtimer, secs));
}

void msleep(uint32_t ms) {
    vrtimer_comp_t *vrt = get_vrtimer();
    _sleep(vrt, MS_TO_TICK(vrt->hrtimer, ms));
}

void usleep(uint32_t us) {
    vrtimer_comp_t *vrt = get_vrtimer();
    _sleep(vrt, US_TO_TICK(vrt->hrtimer, us));
}

/**
 * NOTES:
 *  1. left.secs >= 0 && right.secs >=0
 *  2. left.ns < NSEC_PER_SEC && right.ns < NSEC_PER_SEC
 */
void time_add(time_t *left, time_t *right, time_t *res) {
    res->secs = left->secs + right->secs;
    res->ns = left->ns + right->ns;
    if (res->ns >= NSEC_PER_SEC) {
        res->secs++;
        res->ns -= NSEC_PER_SEC;
    }

    // On overflow, use max value
    if (res->secs < left->secs || res->secs < right->secs) {
        res->secs = S64_MAX;
        res->ns = 0;
    }
}

/**
 * NOTES:
 *  1. left.secs >= 0 && right.secs >=0
 *  2. left.ns < NSEC_PER_SEC && right.ns < NSEC_PER_SEC
 */
void time_sub(time_t *left, time_t *right, time_t *res) {
    res->secs = left->secs - right->secs;
    if (right->ns > left->ns) {
        res->ns = NSEC_PER_SEC + left->ns - right->ns;
        res->secs--;
    } else {
        res->ns = left->ns - right->ns;
    }
}



#ifdef CONFIG_TEST_CORE_TIME_TIME
#include __FILE__
#endif
