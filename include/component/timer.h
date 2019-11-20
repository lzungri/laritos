#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <component/component.h>
#include <component/intc.h>
#include <timer/tick.h>


typedef enum {
    TIMER_EXP_ABSOLUTE = 0,
    TIMER_EXP_RELATIVE,
} timer_exp_type_t;

struct timer_comp;

typedef int (*timer_cb_t)(struct timer_comp *t, void *data);

typedef struct {
    bool enabled;
    int64_t ticks;
    bool periodic;
    timer_cb_t cb;
    void *data;
} timer_t;

typedef struct {
    int (*get_value)(struct timer_comp *t, uint64_t *v);
    int (*set_value)(struct timer_comp *t, uint64_t v);
    int (*get_remaining)(struct timer_comp *t, int64_t *v);
    int (*reset)(struct timer_comp *t);
    int (*set_enable)(struct timer_comp *t, bool enable);

    /**
     * Sets the expiration value for the timer
     *
     * @param t: Timer reference
     * @param ticks: Timer ticks (not OS ticks) to wait for expiration
     * @param type: Whether the specified expiration time is absolute or relative
     *
     * @return 0 on success, <0 on error
     */
    int (*set_expiration_ticks)(struct timer_comp *t, int64_t timer_ticks, timer_exp_type_t type,
            timer_cb_t cb, void *data, bool periodic);
    int (*clear_expiration)(struct timer_comp *t);
} timer_comp_ops_t;

typedef struct timer_comp {
    component_t parent;

    irq_t irq;
    bool intio;
    irq_trigger_mode_t irq_trigger;
    intc_t *intc;
    irq_handler_t irq_handler;

    uint32_t curfreq;
    uint32_t maxfreq;

    timer_t curtimer;

    timer_comp_ops_t ops;
} timer_comp_t;

int timer_init(timer_comp_t *t);
int timer_deinit(timer_comp_t *t);
int timer_component_init(timer_comp_t *t, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));

irqret_t timer_handle_expiration(timer_comp_t *t);
