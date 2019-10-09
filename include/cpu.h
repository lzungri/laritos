#pragma once

#include <stdint.h>
#include <core.h>
#include <component/cpu.h>
#include <component/component.h>

typedef uint32_t cpubits_t;

#define CPU_ALL_MASK ((cpubits_t) -1)
#define BIT_FOR_CPU(_n) ((cpubits_t) (1 << _n))

static inline int cpu_get_id(void) {
    // TODO
    return 0;
}

static inline cpu_t *cpu(void) {
    component_t *c;
    for_each_component_type(c, COMP_TYPE_CPU) {
        cpu_t *cpu = (cpu_t *) c;
        if (cpu->id == cpu_get_id()) {
            return cpu;
        }
    }
    return NULL;
}
