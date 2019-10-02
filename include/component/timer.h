#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <timer.h>
#include <component/component.h>
#include <component/intc.h>

struct timer_comp;
typedef struct {
    int (*get_value)(struct timer_comp *t, uint64_t *v);
    int (*set_value)(struct timer_comp *t, uint64_t v);
    int (*reset)(struct timer_comp *t);
    int (*set_enable)(struct timer_comp *t, bool enable);
    int (*set_expiration)(struct timer_comp *t, int64_t secs, int64_t ns, timer_exp_type_t type);
} timer_comp_ops_t;

typedef struct timer_comp {
    component_t parent;

    irq_t irq;
    bool intio;
    irq_trigger_mode_t irq_trigger;
    intc_t *intc;
    irq_handler_t irq_handler;

    uint64_t resolution_ns;

    timer_comp_ops_t ops;
} timer_comp_t;

int timer_init(timer_comp_t *t);
int timer_deinit(timer_comp_t *t);
int timer_component_init(timer_comp_t *t, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
