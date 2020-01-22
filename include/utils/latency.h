#pragma once

#include <log.h>
#include <stdint.h>
#include <limits.h>
#include <cpu/cpu.h>
#include <time/time.h>
#include <irq/irq.h>
#include <time/tick.h>

/**
 * Utility for benchmarking portions of code.
 * The values shown represent the number of cpu cycles.
 *
 * Format:
 *      #num_call _tag | current minimum maximum average (~average_time)
 *
 * @param _tag: Label
 * @param _expr: Portion of code to benchmark
 */
#define LATENCY(_tag, _expr) do { \
        static uint64_t _diffsum = 0; \
        static uint32_t _count = 0; \
        static uint64_t _min = U64_MAX; \
        static uint64_t _max = 0; \
        uint64_t _start = cpu_get_cycle_count(); \
            (_expr); \
        uint64_t _diff = cpu_get_cycle_count() - _start; \
        _diffsum += _diff; \
        _count++; \
        if (_diff > _max) { \
            _max = _diff; \
        } \
        if (_diff < _min) { \
            _min = _diff; \
        } \
        uint64_t _avg = _diffsum / _count; \
        if (_avg > U32_MAX) { \
            log_always_async("#%lu " _tag " | %lu<<32 %lu<<32 %lu<<32 %lu<<32 (~%lums)", \
                _count, \
                (uint32_t) (_diff >> 32), \
                (uint32_t) (_min >> 32), \
                (uint32_t) (_max >> 32), \
                (uint32_t) (_avg >> 32), \
                (uint32_t) ((_avg * MSEC_PER_SEC) / cpu()->freq)); \
        } else { \
            char *_unit = "us"; \
            uint64_t _units_per_sec = USEC_PER_SEC; \
            if (CPUTICK_TO_SEC(_avg) > 0) { \
                _unit = "ms"; \
                _units_per_sec = MSEC_PER_SEC; \
            } else if (CPUTICK_TO_US(_avg) == 0) { \
                _unit = "ns"; \
                _units_per_sec = NSEC_PER_SEC; \
            } \
            log_always_async("#%lu " _tag " | %lu %lu %lu %lu (~%lu%s)", \
                _count, \
                (uint32_t) _diff, \
                (uint32_t) _min, \
                (uint32_t) _max, \
                (uint32_t) _avg, \
                (uint32_t) ((_avg * _units_per_sec) / cpu()->freq), \
                _unit); \
        } \
    } while (0);

/**
 * NOTE: Make sure you don't use any return/goto statement in your code,
 * otherwise the IRQ mask may not be restored.
 */
#define LATENCY_NOIRQ(_tag, _expr) do { \
        irqctx_t _ctx; \
        irq_disable_local_and_save_ctx(&_ctx); \
        LATENCY(_tag, _expr); \
        irq_local_restore_ctx(&_ctx); \
    } while (0);
