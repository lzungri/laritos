#pragma once

#include <stdint.h>

#include <component/component.h>
#include <generated/autoconf.h>

struct cpu;
typedef struct {
    int (*set_irqs_enable)(struct cpu *c, bool enabled);
} cpu_ops_t;

typedef struct cpu {
    component_t parent;

    uint8_t id;
    struct intc *intc;

    cpu_ops_t ops;
} cpu_t;

int cpu_component_init(cpu_t *c, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
