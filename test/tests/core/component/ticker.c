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
#include <component/ticker.h>
#include <time/tick.h>
#include <time/system-tick.h>
#include <component/component.h>
#include <dstruct/list.h>
#include <test/utils/process.h>
#include <test/utils/time.h>


static bool is_callback_registered(ticker_comp_t *t, ticker_cb_t cb, void *data) {
    ticker_cb_info_t *ti;
    list_for_each_entry(ti, &t->cbs, list) {
        if (ti->cb == cb && ti->data == data) {
            return true;
        }
    }
    return false;
}

T(ticker_global_ctx_tick_is_incremented_periodically_secs) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    int i;
    for (i = 2; i < 30; i += 10) {
        abstick_t ticks = tick_get_os_ticks();
        sleep(i);
        uint32_t delta = OSTICK_TO_SEC(tick_get_os_ticks() - ticks);
        tassert(delta <= i);
        // 20% tolerance for lower limit
        tassert(delta >= (i * 80) / 100);
    }
TEND

T(ticker_global_ctx_tick_is_incremented_periodically_msecs) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    int i;
    for (i = max(1000 / t->ticks_per_sec, 100); i < 1000; i += 100) {
        abstick_t ticks = tick_get_os_ticks();
        msleep(i);
        uint32_t delta = OSTICK_TO_MS(tick_get_os_ticks() - ticks);
        tassert(delta <= i);
        // 30% tolerance for lower limit
        tassert(delta >= (i * 70) / 100);
    }
TEND

T(ticker_can_be_paused_and_resumed) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    abstick_t ticks = tick_get_os_ticks();
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks < tick_get_os_ticks());

    t->ops.pause(t);
    ticks = tick_get_os_ticks();
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks == tick_get_os_ticks());

    t->ops.resume(t);
    ticks = tick_get_os_ticks();
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks <= tick_get_os_ticks());
TEND

static int cb0(ticker_comp_t *t, void *data) {
    return 0;
}

T(ticker_can_add_and_remove_callbacks) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    t->ops.add_callback(t, cb0, NULL);
    tassert(is_callback_registered(t, cb0, NULL));

    // Try to remove the wrong callback
    t->ops.remove_callback(t, cb0, t);
    tassert(is_callback_registered(t, cb0, NULL));
    t->ops.remove_callback(t, NULL, NULL);
    tassert(is_callback_registered(t, cb0, NULL));

    t->ops.remove_callback(t, cb0, NULL);
    tassert(!is_callback_registered(t, cb0, NULL));
TEND

static int cb1(ticker_comp_t *t, void *data) {
    *((bool *) data) = true;
    return 0;
}

T(ticker_callbacks_are_called) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    bool cb_called = false;

    t->ops.add_callback(t, cb1, &cb_called);
    tassert(is_callback_registered(t, cb1, &cb_called));

    TEST_BUSY_WAIT_WHILE(!cb_called, t->ticks_per_sec == 1 ? 2 : 1);

    tassert(cb_called);

    t->ops.remove_callback(t, cb1, &cb_called);
    tassert(!is_callback_registered(t, cb1, &cb_called));
TEND

static abstick_t cb2_ticks = 0;
static int cb2(ticker_comp_t *t, void *data) {
    bool *valid_tick = (bool *) data;
    if (cb2_ticks == 0) {
        // Initialize boolean used for validation
        *valid_tick = true;
    } else if (tick_get_os_ticks() != cb2_ticks + 1) {
        *valid_tick = false;
    }
    cb2_ticks = tick_get_os_ticks();
    return 0;
}

T(ticker_callbacks_are_called_on_every_tick) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    bool valid_tick = false;
    t->ops.add_callback(t, cb2, &valid_tick);
    tassert(is_callback_registered(t, cb2, &valid_tick));

    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 5 : 1);

    tassert(valid_tick);

    t->ops.remove_callback(t, cb2, &valid_tick);
    tassert(!is_callback_registered(t, cb2, &valid_tick));
TEND
