#pragma once

#include <stdint.h>
#include <time/time.h>

/**
 * Type representing a relative amount of ticks
 */
typedef uint32_t tick_t;

/**
 * Type used to deal with absolute tick values, e.g. uptime
 */
typedef uint64_t abstick_t;

#define SEC_TO_TICK(_timer, _secs) ((_secs) * (_timer)->curfreq)
#define MS_TO_TICK(_timer, _ms)  (((_ms) * (_timer)->curfreq) / MSEC_PER_SEC)
#define US_TO_TICK(_timer, _us)  (((_us) * (_timer)->curfreq) / USEC_PER_SEC)

#define TICK_TO_SEC(_timer, _ticks) ((_ticks) / (_timer)->curfreq)
#define TICK_TO_MS(_timer, _ticks) (((_ticks) * MSEC_PER_SEC) / (_timer)->curfreq)
#define TICK_TO_US(_timer, _ticks) (((_ticks) * USEC_PER_SEC) / (_timer)->curfreq)
