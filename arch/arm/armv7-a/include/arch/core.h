#pragma once

#include <stdint.h>

typedef struct {
    /**
     * High-order (leftmost) 32-bits of a 64-bits cycle counter.
     * The low-order is stored in the cpu PMCCNTR register.
     * This value is incremented by one on each PMCCNTR overflow.
     *
     * 64-bits cycle counter = (((uint64_t) high_cycle_counter) << 32) | PMCCNTR
     */
    uint32_t high_cycle_counter;
} arch_data_t;
