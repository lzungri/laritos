#pragma once

#include <log.h>
#include <stdint.h>
#include <limits.h>
#include <sync/atomic.h>
#include <utils/assert.h>

typedef struct refcount {
    atomic32_t value;
    void (*free)(struct refcount *ref);
} refcount_t;

static inline void ref_init(refcount_t *ref, void (*freecb)(refcount_t *ref)) {
    atomic32_init(&ref->value, 0);
    ref->free = freecb;
}

static inline uint32_t ref_get(refcount_t *ref) {
    return atomic32_get(&ref->value);
}

static inline uint32_t ref_inc(refcount_t *ref) {
    verbose_async("ref=0x%p inc", ref);
    int32_t v = atomic32_inc(&ref->value);
    assert(v > 0, "Invalid value for reference counter at 0x%p, value=%ld", ref, v);
    return v;
}

static inline uint32_t ref_dec(refcount_t *ref) {
    verbose_async("ref=0x%p dec", ref);
    int32_t v = atomic32_dec(&ref->value);
    assert(v >= 0, "Reference counter at 0x%p cannot be negative, value=%ld", ref, v);
    if (v == 0 && ref->free != NULL) {
        verbose_async("refcount = 0, freeing objected referenced by ref=0x%p", ref);
        ref->free(ref);
    }
    return v;
}
