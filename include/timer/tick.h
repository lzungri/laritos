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

#define SEC_TO_TICK(_ticker, _secs) ((_secs) * (_ticker)->ticks_per_sec)
#define MS_TO_TICK(_ticker, _ms)  (((_ms) * (_ticker)->ticks_per_sec) / MSEC_PER_SEC)
#define US_TO_TICK(_ticker, _us)  (((_us) * (_ticker)->ticks_per_sec) / USEC_PER_SEC)
