#pragma once

#include <stdint.h>
#include <board-types.h>
#include <dstruct/list.h>
#include <component/component.h>
#include <component/timer.h>

struct ticker_comp;

typedef int (*ticker_cb_t)(struct ticker_comp *t, void *data);

typedef struct {
    ticker_cb_t cb;
    void *data;
} ticker_cb_info_t;

typedef struct {
    int (*add_callback)(struct ticker_comp *t, ticker_cb_t cb, void *data);
} ticker_comp_ops_t;

typedef struct ticker_comp {
    component_t parent;

    uint32_t ticks_per_sec;
    timer_comp_t *timer;
    struct list_head cbs;

    ticker_comp_ops_t ops;
} ticker_comp_t;

int ticker_init(ticker_comp_t *t);
int ticker_deinit(ticker_comp_t *t);
int ticker_component_init(ticker_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
