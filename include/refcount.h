#pragma once

#include <log.h>
#include <stdint.h>
#include <limits.h>
#include <utils/assert.h>

typedef struct refcount {
    uint32_t value;
    void (*free)(struct refcount *ref);
} refcount_t;

static inline void ref_init(refcount_t *ref, void (*freecb)(refcount_t *ref)) {
    ref->value = 0;
    ref->free = freecb;
}

static inline uint32_t ref_get(refcount_t *ref) {
    return ref->value;
}

static inline uint32_t ref_inc(refcount_t *ref) {
    verbose_async("ref=0x%p inc", ref);
    assert(ref->value < U32_MAX, "Reference counter at 0x%p cannot be bigger than %lu", ref, U32_MAX);
    return ref->value++;
}

static inline uint32_t ref_dec(refcount_t *ref) {
    verbose_async("ref=0x%p dec", ref);
    assert(ref->value > 0, "Reference counter at 0x%p cannot be negative", ref);
    if (--ref->value == 0 && ref->free != NULL) {
        verbose_async("refcount = 0, freeing objected referenced by ref=0x%p", ref);
        ref->free(ref);
    }
    return ref->value;
}
