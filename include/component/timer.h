#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <component/component.h>
#include <component/intc.h>

struct timer;
typedef struct {

} timer_ops_t;

typedef struct timer {
    component_t parent;

    irq_t irq;
    bool intio;
    irq_trigger_mode_t irq_trigger;
    intc_t *intc;
    irq_handler_t irq_handler;

    timer_ops_t ops;
} timer_t;

int timer_init(timer_t *t);
int timer_deinit(timer_t *t);
int timer_component_init(timer_t *t, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
