#include <log.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <test/test.h>
#include <component/vrtimer.h>
#include <timer/tick.h>
#include <component/component.h>
#include <dstruct/list.h>


static vrtimer_comp_t *get_vrtimer(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_VRTIMER) {
        return (vrtimer_comp_t *) c;
    }
    return NULL;
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

static int cb0(vrtimer_comp_t *t, void *data) {
    return 0;
}

T(vrtimer_can_add_and_remove_timers) {
    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    t->ops.add_vrtimer(t, 0, cb0, NULL, true);
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
    abstick_t *ticks = (abstick_t *) data;
    *ticks = _laritos.timeinfo.ticks;
    return 0;
}

T(vrtimer_callback_is_called_after_n_ticks) {
    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    tick_t n = 3;
    abstick_t saved_ticks = _laritos.timeinfo.ticks;
    abstick_t cb_ticks = 0;

    t->ops.add_vrtimer(t, n, cb1, &cb_ticks, false);
    tassert(is_vrtimer_registered(t, cb1, &cb_ticks, false));

    while(cb_ticks == 0 && _laritos.timeinfo.ticks < saved_ticks + 2 * n);

    tassert(cb_ticks >= saved_ticks + n);

    // Timer should be removed automatically once expired
    tassert(!is_vrtimer_registered(t, cb1, &cb_ticks, false));
TEND

static abstick_t cb2_ticks = 0;
static tick_t cb2_n = 3;
static int cb2(vrtimer_comp_t *t, void *data) {
    bool *valid_tick = (bool *) data;
    if (cb2_ticks == 0) {
        // Initialize boolean used for validation
        *valid_tick = true;
    } else if (_laritos.timeinfo.ticks != cb2_ticks + cb2_n) {
        *valid_tick = false;
    }
    cb2_ticks = _laritos.timeinfo.ticks;
    return 0;
}

T(vrtimer_callbacks_are_called_on_every_n_ticks_if_vrtimer_is_periodic) {
    vrtimer_comp_t *t = get_vrtimer();
    tassert(t != NULL);

    abstick_t deadline = _laritos.timeinfo.ticks + 3 * cb2_n;
    bool valid_tick = false;

    t->ops.add_vrtimer(t, cb2_n, cb2, &valid_tick, true);
    tassert(is_vrtimer_registered(t, cb2, &valid_tick, true));

    while(_laritos.timeinfo.ticks < deadline);

    tassert(valid_tick);

    // Timer should still be there since it is periodic
    tassert(is_vrtimer_registered(t, cb2, &valid_tick, true));

    // Let's remove it
    t->ops.remove_vrtimer(t, cb2, &valid_tick, true);
    tassert(!is_vrtimer_registered(t, cb2, &valid_tick, true));
TEND
