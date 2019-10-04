#pragma once

#include <stdint.h>

typedef uint32_t cpubits_t;

#define CPU_ALL_MASK ((cpubits_t) -1)
#define BIT_FOR_CPU(_n) ((cpubits_t) (1 << _n))

static inline int cpu_get_id(void) {
    // TODO
    return 0;
}
