#include <log.h>

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/component.h>
#include <component/ticker.h>
#include <component/vrtimer.h>
#include <time/time.h>
#include <time/tick.h>
#include <utils/math.h>


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

T(time_msleep_blocks_kernel_then_wakes_it_up) {
    pause_ticker();
    vrtimer_comp_t *vrt = get_vrtimer();
    int i;
    for (i = 100; i < 1000; i += 100) {
        debug("Sleeping for %u msecs", i);
        abstick_t ticks = get_timer_cur_value(vrt);
        msleep(i);
        abstick_t now = get_timer_cur_value(vrt);
        tassert(now > ticks);
        tassert((now - ticks) >= MS_TO_TICK(vrt->hrtimer, i));
    }
    resume_ticker();
TEND

T(time_usleep_blocks_kernel_then_wakes_it_up) {
    pause_ticker();
    vrtimer_comp_t *vrt = get_vrtimer();
    int i;
    for (i = 100; i < 1000; i += 100) {
        debug("Sleeping for %u usecs", i);
        abstick_t ticks = get_timer_cur_value(vrt);
        usleep(i);
        abstick_t now = get_timer_cur_value(vrt);
        tassert(now > ticks);
        tassert((now - ticks) >= US_TO_TICK(vrt->hrtimer, i));
    }
    resume_ticker();
TEND

static int sleepproc(void *data) {
    sleep(5);
    return 0;
}

static pcb_t *sleep_proc;
static int sleepparent(void *data) {
    sleep_proc = process_spawn_kernel_process("sleep", sleepproc, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    process_wait_for(sleep_proc, NULL);
    return 0;
}

T(time_killing_a_proc_while_sleeping_doesnt_free_its_pcb_due_to_refcount) {
    pcb_t *p = process_spawn_kernel_process("sleepparent", sleepparent, NULL,
                        8196,  process_get_current()->sched.priority - 2);
    tassert(p != NULL);

    schedule();

    tassert(sleep_proc != NULL);
    process_kill(sleep_proc);

    process_wait_for(sleep_proc, NULL);
TEND

T(time_add_handles_overflows_correctly) {
    time_t t, t2, res;
    t.secs = S64_MAX;
    t.ns = 10;
    t2.secs = S64_MAX;
    t2.ns = 0;
    time_add(&t, &t2, &res);
    tassert(res.secs == S64_MAX);
    tassert(res.ns == 0);

    t.secs = S64_MAX;
    t.ns = 10;
    t2.secs = 10;
    t2.ns = 0;
    time_add(&t, &t2, &res);
    tassert(res.secs == S64_MAX);
    tassert(res.ns == 0);

    t.secs = 0;
    t.ns = NSEC_PER_SEC - 1;
    t2.secs = 0;
    t2.ns = NSEC_PER_SEC - 1;
    time_add(&t, &t2, &res);
    tassert(res.secs == 1);
    tassert(res.ns == NSEC_PER_SEC - 2);

    t.secs = 0;
    t.ns = NSEC_PER_SEC - 1;
    t2.secs = 0;
    t2.ns = 10;
    time_add(&t, &t2, &res);
    tassert(res.secs == 1);
    tassert(res.ns == 9);
TEND

T(time_sub_handles_overflows_correctly) {
    time_t t, t2, res;
    t.secs = 3;
    t.ns = 0;
    t2.secs = 1;
    t2.ns = 1;
    time_sub(&t, &t2, &res);
    tassert(res.secs == 1);
    tassert(res.ns == NSEC_PER_SEC - 1);

    t.secs = 5;
    t.ns = 10;
    t2.secs = 0;
    t2.ns = NSEC_PER_SEC - 1;
    time_sub(&t, &t2, &res);
    tassert(res.secs == 4);
    tassert(res.ns == 11);
TEND

T(time_get_monotonic_returns_time_with_the_expected_res_in_usecs) {
    time_t t, t2, res;
    int i;
    for (i = 10; i < 1000; i += 100) {
        time_get_monotonic_time(&t);
        usleep(i);
        time_get_monotonic_time(&t2);
        time_sub(&t2, &t, &res);
        tassert(res.secs == US_TO_SEC(i));
        // ns == US_TO_NS(i) +/- 10 msecs (yeah... pretty bad)
        tassert(nearly_equal(res.ns, US_TO_NS(i), MS_TO_NS(10)));
    }
TEND

T(time_get_monotonic_returns_time_with_the_expected_res_in_msecs) {
    time_t t, t2, res;
    int i;
    for (i = 10; i < 1000; i += 100) {
        time_get_monotonic_time(&t);
        msleep(i);
        time_get_monotonic_time(&t2);
        time_sub(&t2, &t, &res);
        tassert(res.secs == MS_TO_SEC(i));
        // ns == MS_TO_NS(i) +/- 10 msecs
        tassert(nearly_equal(res.ns, MS_TO_NS(i), MS_TO_NS(10)));
    }
TEND

T(time_get_monotonic_returns_time_with_the_expected_res_in_secs) {
    time_t t, t2, res;
    int i;
    for (i = 1; i < 6; i++) {
        time_get_monotonic_time(&t);
        sleep(i);
        time_get_monotonic_time(&t2);
        time_sub(&t2, &t, &res);
        // secs == i +/- 1 sec
        tassert(nearly_equal(res.secs, i, 1));
        tassert(res.ns < NSEC_PER_SEC);
    }
TEND

T(time_get_ns_rtc_returns_time_with_the_expected_res_in_usecs) {
    time_t t, t2, res;
    int i;
    for (i = 10; i < 1000; i += 100) {
        time_get_ns_rtc_time(&t);
        usleep(i);
        time_get_ns_rtc_time(&t2);
        time_sub(&t2, &t, &res);
        tassert(res.secs == US_TO_SEC(i));
        // ns == US_TO_NS(i) +/- 10 msecs (yeah... pretty bad)
        tassert(nearly_equal(res.ns, US_TO_NS(i), MS_TO_NS(10)));
    }
TEND

T(time_get_ns_rtc_returns_time_with_the_expected_res_in_msecs) {
    time_t t, t2, res;
    int i;
    for (i = 1; i < 6; i++) {
        time_get_ns_rtc_time(&t);
        msleep(i);
        time_get_ns_rtc_time(&t2);
        time_sub(&t2, &t, &res);
        tassert(res.secs == MS_TO_SEC(i));
        // ns == MS_TO_NS(i) +/- 10 msecs
        tassert(nearly_equal(res.ns, MS_TO_NS(i), MS_TO_NS(10)));
    }
TEND

T(time_get_ns_rtc_returns_time_with_the_expected_res_in_secs) {
    time_t t, t2, res;
    int i;
    for (i = 1; i < 6; i++) {
        time_get_ns_rtc_time(&t);
        sleep(i);
        time_get_ns_rtc_time(&t2);
        time_sub(&t2, &t, &res);
        // secs == i +/- 1 sec
        tassert(nearly_equal(res.secs, i, 1));
        tassert(res.ns < NSEC_PER_SEC);
    }
TEND
