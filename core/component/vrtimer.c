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

static int vrtimer_cb(timer_comp_t *t, void *data);

static void update_expiration(vrtimer_comp_t *t) {
    if (list_empty(&t->timers)) {
        t->hrtimer->ops.clear_expiration(t->hrtimer);
        t->low_power_timer->ops.clear_expiration(t->low_power_timer);
        return;
    }

    vrtimer_t *vrt = list_first_entry(&t->timers, vrtimer_t, list);

    // Remaining ticks to expire
    int64_t deltaticks;
    t->hrtimer->ops.get_value(t->hrtimer, &deltaticks);
    deltaticks = vrt->abs_ticks - deltaticks;

    // If already expired, execute vrtimer callback and return
    if (deltaticks <= 0) {
        vrtimer_cb(t->hrtimer, t);
        return;
    }

    // If the ticks-to-expire value is lower that the high-res timer frequency
    // (i.e. we need to wake up in less than a second), then use the hrtimer.
    // Otherwise, use the low power timer, since we don't need that much precision
    if ((uint32_t) deltaticks <= t->hrtimer->curfreq) {
        t->low_power_timer->ops.clear_expiration(t->low_power_timer);

        t->hrtimer->ops.set_expiration_ticks(t->hrtimer, vrt->abs_ticks,
                TIMER_EXP_ABSOLUTE, vrtimer_cb, t, false);
    } else {
        t->hrtimer->ops.clear_expiration(t->hrtimer);

        // Round up and normalize ticks for the low power timer
        abstick_t norm_ticks = ((deltaticks + t->hrtimer->curfreq / 2) / t->hrtimer->curfreq) * t->low_power_timer->curfreq;
        t->low_power_timer->ops.set_expiration_ticks(t->low_power_timer, norm_ticks,
                TIMER_EXP_RELATIVE, vrtimer_cb, t, false);
    }
}

// TODO: We should use a rbtree here instead
static void add_vrtimer_sorted(vrtimer_comp_t *t, vrtimer_t *vrt) {
    verbose_async("Adding vrtimer id=0x%p abs_ticks=%lu, ticks=%lu, periodic=%u", vrt, (uint32_t) vrt->abs_ticks, vrt->ticks, vrt->periodic);

    vrtimer_t *pos;
    list_for_each_entry(pos, &t->timers, list) {
        if (vrt->abs_ticks <= pos->abs_ticks) {
            list_add(&vrt->list, pos->list.prev);
            return;
        }
    }
    // New timer has the largest expiration time, add it to the tail
    list_add_tail(&vrt->list, &t->timers);
}

static int vrtimer_cb(timer_comp_t *t, void *data) {
    vrtimer_comp_t *vrt = (vrtimer_comp_t *) data;
    vrtimer_t *pos;
    vrtimer_t *tmp;

    abstick_t curticks;
    t->ops.get_value(t, &curticks);

    list_for_each_entry_safe(pos, tmp, &vrt->timers, list) {
        if (curticks >= pos->abs_ticks) {
            verbose_async("vrtimer id=0x%p abs_ticks=%lu expired", pos, (uint32_t) pos->abs_ticks);

            // Execute timer callback
            if (pos->cb(vrt, pos->data) < 0) {
                error_async("Error while executing vrtimer callback at 0x%p", pos->cb);
            }

            list_del_init(&pos->list);
            if (pos->periodic) {
                pos->abs_ticks = curticks + pos->ticks;
                add_vrtimer_sorted(vrt, pos);
            } else {
                free(pos);
            }
        } else {
            break;
        }
    }

    update_expiration(vrt);

    return 0;
}

static int add_vrtimer(vrtimer_comp_t *t, tick_t ticks, vrtimer_cb_t cb, void *data, bool periodic) {
    vrtimer_t *vrt = calloc(1, sizeof(vrtimer_t));
    if (vrt == NULL) {
        error("Couldn't allocate memory for vrtimer_t");
        return -1;
    }

    t->hrtimer->ops.get_value(t->hrtimer, &vrt->abs_ticks);
    vrt->abs_ticks += ticks;
    vrt->ticks = ticks;
    vrt->periodic = periodic;
    vrt->cb = cb;
    vrt->data = data;
    INIT_LIST_HEAD(&vrt->list);

    add_vrtimer_sorted(t, vrt);

    // If the recently added timer is the soonest to be expired, then update
    // the timer expiration
    update_expiration(t);

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

int vrtimer_init(vrtimer_comp_t *t) {
    INIT_LIST_HEAD(&t->timers);
    return 0;
}

int vrtimer_deinit(vrtimer_comp_t *t) {
    return 0;
}

int vrtimer_component_init(vrtimer_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, COMP_TYPE_VRTIMER, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }

    t->ops.add_vrtimer = add_vrtimer;
    t->ops.remove_vrtimer = remove_vrtimer;

    if (board_get_component_attr(bcomp, "hrtimer", (component_t **) &t->hrtimer) < 0) {
        error("Invalid or no timer component specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "low_power_timer", (component_t **) &t->low_power_timer) < 0) {
        error("Invalid or no low power timer component specified in the board info");
        return -1;
    }

    return 0;
}



#ifdef CONFIG_TEST_CORE_COMPONENT_VRTIMER
#include __FILE__
#endif
