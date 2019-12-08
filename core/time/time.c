#define DEBUG
#include <log.h>

#include <cpu.h>
#include <core.h>
#include <time/time.h>
#include <time/tick.h>
#include <time/timeconv.h>
#include <component/timer.h>
#include <component/vrtimer.h>
#include <component/component.h>
#include <utils/assert.h>
#include <process/core.h>
#include <sched/core.h>

int time_rtc_gettime(time_t *t) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_RTC) {
        timer_comp_t *timer = (timer_comp_t *) c;
        t->ns = 0;
        return timer->ops.get_value(timer, &t->secs);
    }
    return -1;
}

int time_rtc_get_localtime_calendar(calendar_t *c) {
    time_t time = { 0 };
    if (time_rtc_gettime(&time) < 0) {
        return -1;
    }
    return epoch_to_localtime_calendar(time.secs, c);
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


static inline vrtimer_comp_t *get_vrtimer(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_VRTIMER) {
        return (vrtimer_comp_t *) c;
    }
    assert(false, "Cannot use xsleep() family without a virtual timer");
    return NULL;
}

static int process_sleep_cb(vrtimer_comp_t *t, void *data) {
    pcb_t *pcb = (pcb_t *) data;
    sched_move_to_ready(pcb);
    return 0;
}

static int non_process_sleep_cb(vrtimer_comp_t *t, void *data) {
    bool *timer_expired = (bool *) data;
    *timer_expired = true;
    return 0;
}

static inline void _sleep(vrtimer_comp_t *t, tick_t ticks) {
    if (_laritos.process_mode) {
        pcb_t *pcb = pcb_get_current();
        sched_move_to_blocked(pcb);
        // Running in process mode, then block the process and schedule()
        if (t->ops.add_vrtimer(t, ticks, process_sleep_cb, pcb, false) < 0) {
            error_async("Failed to create virtual timer ticks=%lu", ticks);
            return;
        }
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



#ifdef CONFIG_TEST_CORE_TIME_TIME
#include __FILE__
#endif
