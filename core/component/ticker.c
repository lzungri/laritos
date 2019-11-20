#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <board.h>
#include <component/ticker.h>
#include <component/timer.h>
#include <component/component.h>
#include <timer/tick.h>
#include <utils/math.h>


static int ticker_cb(timer_comp_t *t, void *data) {
    verbose_async("Tick");

    // Call CBs
    return 0;
}

int ticker_init(ticker_comp_t *t) {
    tick_t timer_ticks = t->timer->curfreq / t->ticks_per_sec;
    if (timer_ticks == 0) {
        // If the timer resolution cannot handle the required ticks_per_sec, then
        // expire at the next tick
        timer_ticks = 1;
    }
    return t->timer->ops.set_expiration_ticks(t->timer, timer_ticks,
            TIMER_EXP_RELATIVE, ticker_cb, t, true);
}

int ticker_deinit(ticker_comp_t *t) {
    t->timer->ops.clear_expiration(t->timer);
    return 0;
}

static int add_callback(ticker_comp_t *t, ticker_cb_t cb, void *data) {

    // TODO Maybe we can just hardcode the callbacks


    return 0;
}

int ticker_component_init(ticker_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, COMP_TYPE_TICKER, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }

    t->ops.add_callback = add_callback;

    if (board_get_component_attr(bcomp, "timer", (component_t **) &t->timer) < 0) {
        error("Invalid or no timer component specified in the board info");
        return -1;
    }

    board_get_int_attr_def(bcomp, "ticks_per_sec", (int *) &t->ticks_per_sec,
            min(CONFIG_TICKER_DEF_FREQ, t->timer->maxfreq));

    return 0;
}
