#include <log.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/ticker.h>
#include <time/tick.h>
#include <component/component.h>
#include <dstruct/list.h>


static ticker_comp_t *get_ticker(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_TICKER) {
        return (ticker_comp_t *) c;
    }
    return NULL;
}

static bool is_callback_registered(ticker_comp_t *t, ticker_cb_t cb, void *data) {
    ticker_cb_info_t *ti;
    list_for_each_entry(ti, &t->cbs, list) {
        if (ti->cb == cb && ti->data == data) {
            return true;
        }
    }
    return false;
}

T(ticker_global_ctx_tick_is_incremented_periodically) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    abstick_t ticks = _laritos.timeinfo.ticks;

    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);

    tassert(ticks < _laritos.timeinfo.ticks);
TEND

T(ticker_can_be_paused_and_resumed) {
    ticker_comp_t *t = get_ticker();
    tassert(t != NULL);

    abstick_t ticks = _laritos.timeinfo.ticks;
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks < _laritos.timeinfo.ticks);

    t->ops.pause(t);
    ticks = _laritos.timeinfo.ticks;
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks == _laritos.timeinfo.ticks);

    t->ops.resume(t);
    ticks = _laritos.timeinfo.ticks;
    TEST_BUSY_WAIT(t->ticks_per_sec == 1 ? 2 : 1);
    tassert(ticks <= _laritos.timeinfo.ticks);
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
    } else if (_laritos.timeinfo.ticks != cb2_ticks + 1) {
        *valid_tick = false;
    }
    cb2_ticks = _laritos.timeinfo.ticks;
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
