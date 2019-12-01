#pragma once

#include <stdint.h>

#include <component/component.h>
#include <generated/autoconf.h>

struct cpu;
typedef struct {
    int (*set_irqs_enable)(struct cpu *c, bool enabled);
} cpu_ops_t;

struct sched_comp;
typedef struct cpu {
    component_t parent;

    uint8_t id;
    struct intc *intc;
    struct sched_comp *sched;

    cpu_ops_t ops;
} cpu_t;

int cpu_component_init(cpu_t *c, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
