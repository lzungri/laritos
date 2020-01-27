#pragma once

#include <arch/barrier.h>

static inline void dmb(void) {
    arch_barrier_dmb();
}
