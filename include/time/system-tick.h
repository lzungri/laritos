#pragma once

#include <core.h>
#include <sync/atomic.h>
#include <time/tick.h>

static inline void tick_reset_system_ticks(void) {
    return atomic64_init(&_laritos.timeinfo.ticks, 0);
}

static inline abstick_t tick_get_system_ticks(void) {
    return (abstick_t) atomic64_get(&_laritos.timeinfo.ticks);
}

static inline abstick_t tick_inc_system_ticks(void) {
    // Even though the ticks var is an int64_t (atomic64_t) we still treat it
    // as an uint64_t
    return (abstick_t) atomic64_inc(&_laritos.timeinfo.ticks);
}
