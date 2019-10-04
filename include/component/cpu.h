#pragma once

#include <stdint.h>

#include <component/component.h>
#include <generated/autoconf.h>

struct cpu_comp;
typedef struct {
    int (*set_irqs_enable)(struct cpu_comp *c, bool enabled);
} cpu_ops_t;

typedef struct cpu_comp {
    component_t parent;

    uint8_t id;
    struct intc *intc;

    cpu_ops_t ops;
} cpu_comp_t;

int cpu_component_init(cpu_comp_t *c, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
