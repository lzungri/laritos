#define DEBUG
#include <log.h>

#include <stdint.h>
#include <board.h>
#include <core.h>
#include <component/component.h>
#include <component/vrtimer.h>
#include <component/ticker.h>
#include <utils/function.h>
#include <dstruct/list.h>
#include <mm/heap.h>

// TODO: We should use a rbtree here instead
static void add_vrtimer_sorted(vrtimer_comp_t *t, vrtimer_t *vrt) {
    verbose_async("Adding vrtimer abs_ticks=%lu, periodic=%u", (uint32_t) vrt->abs_ticks, vrt->periodic);
    if (list_empty(&t->timers)) {
        list_add(&vrt->list, &t->timers);
        return;
    }

    vrtimer_t *pos;
    list_for_each_entry(pos, &t->timers, list) {
        if (vrt->abs_ticks <= pos->abs_ticks) {
            list_add(&vrt->list, &pos->list);
        }
    }
}

static int add_vrtimer(vrtimer_comp_t *t, tick_t ticks, vrtimer_cb_t cb, void *data, bool periodic) {
    vrtimer_t *vrt = calloc(1, sizeof(vrtimer_t));
    if (vrt == NULL) {
        error("Couldn't allocate memory for vrtimer_t");
        return -1;
    }

    vrt->ticks = ticks;
    vrt->abs_ticks = _laritos.timeinfo.ticks + ticks;
    vrt->periodic = periodic;
    vrt->cb = cb;
    vrt->data = data;
    INIT_LIST_HEAD(&vrt->list);

    add_vrtimer_sorted(t, vrt);
    return 0;
}

static int remove_vrtimer(vrtimer_comp_t *t, vrtimer_cb_t cb, void *data, bool periodic) {
    vrtimer_t *pos;
    vrtimer_t *tmp;
    list_for_each_entry_safe(pos, tmp, &t->timers, list) {
        if (pos->cb == cb && pos->data == data && pos->periodic == periodic) {
            verbose_async("Removing vrtimer with cb=0x%p, data=0x%p, periodic=%u", cb, data, periodic);
            list_del(&pos->list);
            free(pos);
            break;
        }
    }
    return 0;
}

static int vrtimer_ticker_cb(ticker_comp_t *t, void *data) {
    vrtimer_comp_t *vrt = (vrtimer_comp_t *) data;
    vrtimer_t *pos;
    vrtimer_t *tmp;

    list_for_each_entry_safe(pos, tmp, &vrt->timers, list) {
        if (pos->abs_ticks <= _laritos.timeinfo.ticks) {
            verbose_async("vrtimer abs_ticks=%lu expired", (uint32_t) pos->abs_ticks);

            // Execute timer callback
            if (pos->cb(vrt, pos->data) < 0) {
                error_async("Error while executing vrtimer callback at 0x%p", pos->cb);
            }

            list_del_init(&pos->list);
            if (pos->periodic) {
                pos->abs_ticks = _laritos.timeinfo.ticks + pos->ticks;
                add_vrtimer_sorted(vrt, pos);
            } else {
                free(pos);
            }
        } else {
            break;
        }
    }

    return 0;
}

int vrtimer_init(vrtimer_comp_t *t) {
    INIT_LIST_HEAD(&t->timers);
    return t->ticker->ops.add_callback(t->ticker, vrtimer_ticker_cb, t);
}

int vrtimer_deinit(vrtimer_comp_t *t) {
    return t->ticker->ops.remove_callback(t->ticker, vrtimer_ticker_cb, t);
}

int vrtimer_component_init(vrtimer_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, COMP_TYPE_VRTIMER, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }

    t->ops.add_vrtimer = add_vrtimer;
    t->ops.remove_vrtimer = remove_vrtimer;

    if (board_get_component_attr(bcomp, "ticker", (component_t **) &t->ticker) < 0) {
        error("invalid or no ticker component specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "rtc", (component_t **) &t->rtc) < 0) {
        error("invalid or no rtc component specified in the board info");
        return -1;
    }

    return 0;
}
