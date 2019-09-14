#pragma once

#include <stdint.h>

typedef uint32_t cpubits_t;

#define CPU_ALL_MASK ((cpubits_t) -1)

static inline int get_cpu_id(void) {
    // TODO
    return -1;
}
