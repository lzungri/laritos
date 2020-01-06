#pragma once

#include <stdint.h>
#include <arch/atomic.h>

static inline void atomic32_init(atomic32_t *at, int32_t value) {
    arch_atomic32_init(at, value);
}

static inline int32_t atomic32_get(atomic32_t *at) {
    return arch_atomic32_get(at);
}

static inline int32_t atomic32_add(atomic32_t *at, int32_t v) {
    return arch_atomic32_add(at, v);
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


static inline void atomic64_init(atomic64_t *at, int64_t value) {
    arch_atomic64_init(at, value);
}

static inline int64_t atomic64_get(atomic64_t *at) {
    return arch_atomic64_get(at);
}

static inline int64_t atomic64_add(atomic64_t *at, int64_t v) {
    return arch_atomic64_add(at, v);
}

static inline int64_t atomic64_inc(atomic64_t *at) {
    return arch_atomic64_inc(at);
}

static inline int64_t atomic64_dec(atomic64_t *at) {
    return arch_atomic64_dec(at);
}

static inline int64_t atomic64_set(atomic64_t *at, int64_t value) {
    return arch_atomic64_set(at, value);
}
