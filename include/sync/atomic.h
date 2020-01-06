#pragma once

#include <stdint.h>
#include <arch/atomic.h>

static inline void atomic32_init(atomic32_t *at, int32_t value) {
    arch_atomic32_init(at, value);
}

static inline int32_t atomic32_get(atomic32_t *at) {
    return arch_atomic32_get(at);
}

static inline int32_t atomic32_inc(atomic32_t *at) {
    return arch_atomic32_inc(at);
}

static inline int32_t atomic32_dec(atomic32_t *at) {
    return arch_atomic32_dec(at);
}

static inline int32_t atomic32_set(atomic32_t *at, int32_t value) {
    return arch_atomic32_set(at, value);
}
