#pragma once

#include <log.h>
#include <stdint.h>
#include <limits.h>
#include <utils/assert.h>

typedef uint32_t refcount_t;

static inline void ref_inc(refcount_t *ref) {
    verbose_async("INC ref=0x%p", ref);
    assert(*ref < U32_MAX, "Reference counter at 0x%p cannot be bigger than %lu", ref, U32_MAX);
    *ref += 1;
}

static inline void ref_dec(refcount_t *ref) {
    verbose_async("DEC ref=0x%p", ref);
    assert(*ref > 0, "Reference counter at 0x%p cannot be negative", ref);
    *ref -= 1;
}
