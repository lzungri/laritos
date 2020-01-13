#pragma once

#include <stdint.h>
#include <time/time.h>
#include <component/component.h>

/**
 * Type representing a relative amount of ticks
 */
typedef uint32_t tick_t;

/**
 * Type used to deal with absolute tick values, e.g. uptime
 */
typedef uint64_t abstick_t;

#define SEC_TO_TICK(_timer, _secs) ((uint64_t) (_secs) * (_timer)->curfreq)
#define MS_TO_TICK(_timer, _ms)  (((uint64_t) (_ms) * (_timer)->curfreq) / MSEC_PER_SEC)
#define US_TO_TICK(_timer, _us)  (((uint64_t) (_us) * (_timer)->curfreq) / USEC_PER_SEC)

#define TICK_TO_SEC(_timer, _ticks) ((uint64_t) (_ticks) / (_timer)->curfreq)
#define TICK_TO_MS(_timer, _ticks) (((uint64_t) (_ticks) * MSEC_PER_SEC) / (_timer)->curfreq)
#define TICK_TO_US(_timer, _ticks) (((uint64_t) (_ticks) * USEC_PER_SEC) / (_timer)->curfreq)


#define _OSTICKS_PER_SEC (((ticker_comp_t *) component_first_of_type(COMP_TYPE_TICKER))->ticks_per_sec)

#define OSTICK_TO_SEC(_ticks) ((uint64_t) (_ticks) / _OSTICKS_PER_SEC)
#define OSTICK_TO_MS(_ticks) (((uint64_t) (_ticks) * MSEC_PER_SEC) / _OSTICKS_PER_SEC)
#define OSTICK_TO_US(_ticks) (((uint64_t) (_ticks) * USEC_PER_SEC) / _OSTICKS_PER_SEC)
