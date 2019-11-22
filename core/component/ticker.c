#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <board.h>
#include <core.h>
#include <component/ticker.h>
#include <component/timer.h>
#include <component/component.h>
#include <timer/tick.h>
#include <utils/math.h>
#include <dstruct/list.h>
#include <mm/heap.h>


static int ticker_cb(timer_comp_t *t, void *data) {
    // Increment global tick
    _laritos.timeinfo.ticks++;

    ticker_comp_t *ticker = (ticker_comp_t *) data;
    ticker_cb_info_t *ti;
    list_for_each_entry(ti, &ticker->cbs, list) {
        verbose_async("Executing ticker callback 0x%p(data=0x%p)", ti->cb, ti->data);
        if (ti->cb(ticker, ti->data) < 0) {
            error_async("Failed to execute callback 0x%p(data=0x%p)", ti->cb, ti->data);
        }
    }

    return 0;
}

int ticker_init(ticker_comp_t *t) {
    INIT_LIST_HEAD(&t->cbs);

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
    verbose_async("Adding ticker callback 0x%p(data=0x%p)", cb, data);

    ticker_cb_info_t *ti = calloc(1, sizeof(ticker_cb_info_t));
    if (ti == NULL) {
        error("Couldn't allocate memory for ticker_cb_info_t");
        return -1;
    }

    ti->cb = cb;
    ti->data = data;
    INIT_LIST_HEAD(&ti->list);

    list_add_tail(&ti->list, &t->cbs);
    return 0;
}

static int remove_callback(ticker_comp_t *t, ticker_cb_t cb, void *data) {
    ticker_cb_info_t *pos;
    ticker_cb_info_t *tmp;
    list_for_each_entry_safe(pos, tmp, &t->cbs, list) {
        if (pos->cb == cb && pos->data == data) {
            verbose_async("Removing ticker callback 0x%p(data=0x%p)", cb, data);
            list_del(&pos->list);
            free(pos);
            break;
        }
    }
    return 0;
}

int ticker_component_init(ticker_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, COMP_TYPE_TICKER, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }

    t->ops.add_callback = add_callback;
    t->ops.remove_callback = remove_callback;

    if (board_get_component_attr(bcomp, "timer", (component_t **) &t->timer) < 0) {
        error("Invalid or no timer component specified in the board info");
        return -1;
    }

    board_get_int_attr_def(bcomp, "ticks_per_sec", (int *) &t->ticks_per_sec,
            min(CONFIG_TICKER_DEF_FREQ, t->timer->maxfreq));

    if (t->ticks_per_sec == 0) {
        error("Ticks per second cannot be zero");
        return -1;
    }

    return 0;
}



#ifdef CONFIG_TEST_CORE_COMPONENT_TICKER
#include __FILE__
#endif
