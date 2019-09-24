#pragma once

#include <stdint.h>
#include <arch/spinlock.h>

// TODO: Implement
typedef uint16_t spinlock_t;

// TODO Implement this using real sync mechanism!
static inline int spinlock_init(spinlock_t *lock) {
    *lock = 0;
    return 0;
}

static inline int spinlock_ctx_save(spinlock_t *lock, spinctx_t *ctx) {
    while(*lock != 0);
    *lock = 1;
    return *lock;
}

static inline int spinlock_ctx_restore(spinlock_t *lock, spinctx_t *ctx) {
    *lock = 0;
    return *lock;
}
