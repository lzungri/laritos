#pragma once

#include <irq/irq.h>
#include <generated/autoconf.h>

#define DEF_CPU_LOCAL(_type, _name) \
    __typeof__(_type) _name[CONFIG_CPU_MAX_CPUS]

#define CPU_LOCAL_GET(_name) \
    (({ \
        irqctx_t _ctx; \
        irq_disable_local_and_save_ctx(&_ctx); \
        typeof(_name[0]) _value = _name[cpu_get_id()]; \
        irq_local_restore_ctx(&_ctx); \
        _value; \
    }))

#define CPU_LOCAL_SET(_name, _value) do { \
        irqctx_t _ctx; \
        irq_disable_local_and_save_ctx(&_ctx); \
        (_name)[cpu_get_id()] = _value; \
        irq_local_restore_ctx(&_ctx); \
    } while (0)
