#pragma once

#include <core.h>
#include <sync/atomic.h>
#include <time/tick.h>

static inline void tick_reset_os_ticks(void) {
    return atomic64_init(&_laritos.timeinfo.osticks, 0);
}

static inline abstick_t tick_get_os_ticks(void) {
    return (abstick_t) atomic64_get(&_laritos.timeinfo.osticks);
}

static inline abstick_t tick_inc_os_ticks(void) {
    // Even though the ticks var is an int64_t (atomic64_t) we still treat it
    // as an uint64_t
    return (abstick_t) atomic64_inc(&_laritos.timeinfo.osticks);
}
