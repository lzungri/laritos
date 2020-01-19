#pragma once

#include <log.h>
#include <stdint.h>
#include <limits.h>
#include <cpu.h>
#include <time/time.h>
#include <irq.h>

#define LATENCY(_tag, _expr) do { \
        static uint64_t _avg = 0; \
        static uint32_t _count = 0; \
        static uint64_t _min = U64_MAX; \
        static uint64_t _max = 0; \
        uint64_t _start = cpu_get_cycle_count(); \
            (_expr); \
        uint64_t _diff = cpu_get_cycle_count() - _start; \
        _avg += _diff; \
        _count++; \
        if (_diff > _max) { \
            _max = _diff; \
        } \
        if (_diff < _min) { \
            _min = _diff; \
        } \
        log_always_async(_tag " #%lu | cur=%llu min=%llu max=%llu avg=%llu", _count, _diff, _min, _max, _avg / _count); \
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
