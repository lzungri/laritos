#pragma once

#include <stdint.h>
#include <time/core.h>
#include <component/component.h>

/**
 * Type representing a relative amount of ticks
 */
typedef uint32_t tick_t;

/**
 * Type used to deal with absolute tick values, e.g. uptime
 */
typedef uint64_t abstick_t;


#define SEC_TO_TIMER_TICK(_timer, _secs) ((uint64_t) (_secs) * (_timer)->curfreq)
#define MS_TO_TIMER_TICK(_timer, _ms)  (((uint64_t) (_ms) * (_timer)->curfreq) / MSEC_PER_SEC)
#define US_TO_TIMER_TICK(_timer, _us)  (((uint64_t) (_us) * (_timer)->curfreq) / USEC_PER_SEC)

#define TIMER_TICK_TO_SEC(_timer, _ticks) ((uint64_t) (_ticks) / (_timer)->curfreq)
#define TIMER_TICK_TO_MS(_timer, _ticks) (((uint64_t) (_ticks) * MSEC_PER_SEC) / (_timer)->curfreq)
#define TIMER_TICK_TO_US(_timer, _ticks) (((uint64_t) (_ticks) * USEC_PER_SEC) / (_timer)->curfreq)
#define TIMER_TICK_TO_NS(_timer, _ticks) (((uint64_t) (_ticks) * NSEC_PER_SEC) / (_timer)->curfreq)


#define DEF_HRTIMER (component_get_default(COMP_TYPE_VRTIMER, vrtimer_comp_t)->hrtimer)

#define SEC_TO_TICK(_secs) SEC_TO_TIMER_TICK(DEF_HRTIMER, _secs)
#define MS_TO_TICK(_ms) MS_TO_TIMER_TICK(DEF_HRTIMER, _ms)
#define US_TO_TICK(_us) US_TO_TIMER_TICK(DEF_HRTIMER, _us)

#define TICK_TO_SEC(_ticks) TIMER_TICK_TO_SEC(DEF_HRTIMER, _ticks)
#define TICK_TO_MS(_ticks) TIMER_TICK_TO_MS(DEF_HRTIMER, _ticks)
#define TICK_TO_US(_ticks) TIMER_TICK_TO_US(DEF_HRTIMER, _ticks)
#define TICK_TO_NS(_ticks) TIMER_TICK_TO_NS(DEF_HRTIMER, _ticks)


#define _OSTICKS_PER_SEC (component_get_default(COMP_TYPE_TICKER, ticker_comp_t)->ticks_per_sec)

#define OSTICK_TO_SEC(_ticks) ((uint64_t) (_ticks) / _OSTICKS_PER_SEC)
#define OSTICK_TO_MS(_ticks) (((uint64_t) (_ticks) * MSEC_PER_SEC) / _OSTICKS_PER_SEC)
#define OSTICK_TO_US(_ticks) (((uint64_t) (_ticks) * USEC_PER_SEC) / _OSTICKS_PER_SEC)
#define OSTICK_TO_NS(_ticks) (((uint64_t) (_ticks) * NSEC_PER_SEC) / _OSTICKS_PER_SEC)

#define CPUTICK_TO_SEC(_ticks) ((uint64_t) (_ticks) / (cpu()->freq))
#define CPUTICK_TO_MS(_ticks) (((uint64_t) (_ticks) * MSEC_PER_SEC) / (cpu()->freq))
#define CPUTICK_TO_US(_ticks) (((uint64_t) (_ticks) * USEC_PER_SEC) / (cpu()->freq))
#define CPUTICK_TO_NS(_ticks) (((uint64_t) (_ticks) * NSEC_PER_SEC) / (cpu()->freq))
