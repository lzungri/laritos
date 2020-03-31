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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/vrtimer.h>
#include <component/ticker.h>
#include <time/tick.h>
#include <component/component.h>
#include <dstruct/list.h>
#include <test/utils/process.h>
#include <test/utils/time.h>

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

static bool is_vrtimer_registered(vrtimer_comp_t *t, vrtimer_cb_t cb, void *data, bool periodic) {
    vrtimer_t *vrt;
    list_for_each_entry(vrt, &t->timers, list) {
        if (vrt->cb == cb && vrt->data == data && vrt->periodic == periodic) {
            return true;
        }
    }
    return false;
}

static inline abstick_t get_timer_cur_value(vrtimer_comp_t *t) {
    abstick_t cur;
    t->hrtimer->ops.get_value(t->hrtimer, &cur);
    return cur;
}

static int cb0(vrtimer_comp_t *t, void *data) {
    return 0;
}

T(vrtimer_can_add_and_remove_timers) {
    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    t->ops.add_vrtimer(t, 10000000, cb0, NULL, true);
    tassert(is_vrtimer_registered(t, cb0, NULL, true));

    // Try to remove the wrong timer
    t->ops.remove_vrtimer(t, cb0, t, true);
    tassert(is_vrtimer_registered(t, cb0, NULL, true));
    t->ops.remove_vrtimer(t, NULL, NULL, true);
    tassert(is_vrtimer_registered(t, cb0, NULL, true));
    t->ops.remove_vrtimer(t, cb0, NULL, false);
    tassert(is_vrtimer_registered(t, cb0, NULL, true));

    t->ops.remove_vrtimer(t, cb0, NULL, true);
    tassert(!is_vrtimer_registered(t, cb0, NULL, true));
TEND

static int cb1(vrtimer_comp_t *t, void *data) {
    t->hrtimer->ops.get_value(t->hrtimer, (abstick_t *) data);
    return 0;
}

T(vrtimer_callback_is_called_after_n_ticks) {
    pause_ticker();

    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    tick_t n = 3;
    abstick_t cb_ticks = 0;
    abstick_t saved_ticks = get_timer_cur_value(t);

    t->ops.add_vrtimer(t, n, cb1, &cb_ticks, false);

    while(cb_ticks == 0 && get_timer_cur_value(t) < saved_ticks + 2 * n);

    tassert(cb_ticks >= saved_ticks + n);

    // Timer should be removed automatically once expired
    tassert(!is_vrtimer_registered(t, cb1, &cb_ticks, false));

    resume_ticker();
TEND

static abstick_t cb2_ticks = 0;
static tick_t cb2_n;
static int cb2(vrtimer_comp_t *t, void *data) {
    bool *valid_tick = (bool *) data;

    abstick_t cur = get_timer_cur_value(t);
    abstick_t toler = (t->hrtimer->curfreq * 10) / 100;
    abstick_t expected = cb2_ticks + cb2_n;
    if (cb2_ticks == 0) {
        // Initialize boolean used for validation
        *valid_tick = true;
    } else if (cur < expected - toler || cur > expected + toler) {
        *valid_tick = false;
    }
    cb2_ticks = cur;
    return 0;
}

T(vrtimer_callbacks_are_called_on_every_n_ticks_if_vrtimer_is_periodic) {
    pause_ticker();

    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    // 3 seconds period timer
    cb2_n = 3 * t->hrtimer->curfreq;

    // deadline = 12 seconds from now
    abstick_t deadline = get_timer_cur_value(t) + 4 * cb2_n;
    bool valid_tick = false;

    t->ops.add_vrtimer(t, cb2_n, cb2, &valid_tick, true);
    tassert(is_vrtimer_registered(t, cb2, &valid_tick, true));

    while(get_timer_cur_value(t) < deadline);

    // Timer should still be there since it is periodic
    tassert(is_vrtimer_registered(t, cb2, &valid_tick, true));

    // Let's remove it
    t->ops.remove_vrtimer(t, cb2, &valid_tick, true);
    tassert(!is_vrtimer_registered(t, cb2, &valid_tick, true));

    resume_ticker();

    tassert(valid_tick);
TEND
