#pragma once

#include <stdbool.h>
#include <arch/cmpxchg.h>

static inline bool atomic_cmpxchg(volatile int *buf, int old, int new) {
    return arch_atomic_cmpxchg(buf, old, new);
}
