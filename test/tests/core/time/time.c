#include <log.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/component.h>
#include <component/ticker.h>
#include <component/vrtimer.h>
#include <time/time.h>
#include <time/tick.h>


static void pause_ticker(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_TICKER) {
        ticker_comp_t *ticker = (ticker_comp_t *) c;
        ticker->ops.pause(ticker);
    }
}

static void resume_ticker(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_TICKER) {
        ticker_comp_t *ticker = (ticker_comp_t *) c;
        ticker->ops.resume(ticker);
    }
}

static inline abstick_t get_timer_cur_value(vrtimer_comp_t *t) {
    abstick_t cur;
    t->hrtimer->ops.get_value(t->hrtimer, &cur);
    return cur;
}

T(time_sleep_blocks_kernel_then_wakes_it_up) {
    pause_ticker();
    vrtimer_comp_t *vrt = get_vrtimer();
    int i;
    for (i = 1; i < 6; i++) {
        debug("Sleeping for %u secs", i);
        abstick_t ticks = get_timer_cur_value(vrt);
        sleep(i);
        abstick_t now = get_timer_cur_value(vrt);
        tassert(now > ticks);
        tassert((now - ticks) >= SEC_TO_TICK(vrt->hrtimer, i));
    }
    resume_ticker();
TEND

//T(time_msleep_blocks_kernel_then_wakes_it_up) {
//    pause_ticker();
//    vrtimer_comp_t *vrt = get_vrtimer();
//    int i;
//    for (i = 100; i < 1000; i += 100) {
//        debug("Sleeping for %u msecs", i);
//        abstick_t ticks = get_timer_cur_value(vrt);
//        msleep(i);
//        abstick_t now = get_timer_cur_value(vrt);
//        tassert(now > ticks);
//        tassert((now - ticks) >= MS_TO_TICK(vrt->hrtimer, i));
//    }
//    resume_ticker();
//TEND
//
//T(time_usleep_blocks_kernel_then_wakes_it_up) {
//    pause_ticker();
//    vrtimer_comp_t *vrt = get_vrtimer();
//    int i;
//    for (i = 100; i < 1000; i += 100) {
//        debug("Sleeping for %u usecs", i);
//        abstick_t ticks = get_timer_cur_value(vrt);
//        usleep(i);
//        abstick_t now = get_timer_cur_value(vrt);
//        tassert(now > ticks);
//        tassert((now - ticks) >= US_TO_TICK(vrt->hrtimer, i));
//    }
//    resume_ticker();
//TEND
