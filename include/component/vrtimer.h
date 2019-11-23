#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <component/component.h>
#include <component/timer.h>
#include <timer/tick.h>
#include <dstruct/list.h>

struct vrtimer_comp;

typedef int (*vrtimer_cb_t)(struct vrtimer_comp *t, void *data);


typedef struct {
    tick_t ticks;
    abstick_t abs_ticks;
    bool periodic;
    vrtimer_cb_t cb;
    void *data;

    struct list_head list;
} vrtimer_t;

typedef struct {
    int (*add_vrtimer)(struct vrtimer_comp *t, tick_t ticks, vrtimer_cb_t cb, void *data, bool periodic);
    int (*remove_vrtimer)(struct vrtimer_comp *t, vrtimer_cb_t cb, void *data, bool periodic);
} vrtimer_comp_ops_t;

typedef struct vrtimer_comp {
    component_t parent;

    struct list_head timers;
    timer_comp_t *hrtimer;
    timer_comp_t *low_power_timer;

    vrtimer_comp_ops_t ops;
} vrtimer_comp_t;

int vrtimer_init(vrtimer_comp_t *t);
int vrtimer_deinit(vrtimer_comp_t *t);
int vrtimer_component_init(vrtimer_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
