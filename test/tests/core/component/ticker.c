#include <log.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/ticker.h>
#include <timer/tick.h>
#include <component/component.h>
#include <dstruct/list.h>

// TODO Can we just remove this arbitrary value? (without using sleep() since
// we don't if it works at this instance)
#define MAX_BUSY_WAIT_CYCLES 1000000000L

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
    abstick_t ticks = _laritos.timeinfo.ticks;
    uint64_t counter = 0;
    while(ticks == _laritos.timeinfo.ticks && counter < MAX_BUSY_WAIT_CYCLES) {
        counter++;
    }
    tassert(counter < MAX_BUSY_WAIT_CYCLES);
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

    uint64_t counter = 0;
    while(!cb_called && counter < MAX_BUSY_WAIT_CYCLES) {
        counter++;
    }
    tassert(counter < MAX_BUSY_WAIT_CYCLES);

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

    uint64_t counter = 0;
    while(counter < MAX_BUSY_WAIT_CYCLES) {
        counter++;
    }
    tassert(valid_tick);

    t->ops.remove_callback(t, cb2, &valid_tick);
    tassert(!is_callback_registered(t, cb2, &valid_tick));
TEND
