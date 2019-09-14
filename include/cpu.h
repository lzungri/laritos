#pragma once

#include <stdint.h>

typedef uint32_t cpubits_t;

#define CPU_ALL_MASK ((cpubits_t) -1)
#define CPUBIT(_n) ((cpubits_t) (1 << _n))

static inline int get_cpu_id(void) {
    // TODO
    return -1;
}
