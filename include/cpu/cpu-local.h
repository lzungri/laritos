#pragma once

#include <irq/core.h>
#include <utils/assert.h>
#include <utils/utils.h>
#include <generated/autoconf.h>

#define DEF_CPU_LOCAL(_type, _name) \
    __typeof__(_type) _name[CONFIG_CPU_MAX_CPUS]

#define CPU_LOCAL_FOR_EACH_CPU_VAR(_varname, _var) \
    for (_var = _varname; _var <= &_varname[CONFIG_CPU_MAX_CPUS - 1]; _var++)

#define CPU_LOCAL_GET_PTR_LOCKED(_name) \
    (({ \
        uint8_t cpuid = arch_cpu_get_id(); \
        assert(cpuid < ARRAYSIZE(_name), "Invalid cpu id #%u", cpuid); \
        &_name[cpuid]; \
    }))

#define CPU_LOCAL_GET(_name) \
    (({ \
        irqctx_t _ctx; \
        irq_disable_local_and_save_ctx(&_ctx); \
        \
        uint8_t cpuid = arch_cpu_get_id(); \
        assert(cpuid < ARRAYSIZE(_name), "Invalid cpu id #%u", cpuid); \
        typeof(_name[0]) _value = _name[cpuid]; \
        \
        irq_local_restore_ctx(&_ctx); \
        _value; \
    }))

#define CPU_LOCAL_SET(_name, _value) do { \
        irqctx_t _ctx; \
        irq_disable_local_and_save_ctx(&_ctx); \
        \
        uint8_t cpuid = arch_cpu_get_id(); \
        assert(cpuid < ARRAYSIZE(_name), "Invalid cpu id #%u", cpuid); \
        (_name)[cpuid] = _value; \
        \
        irq_local_restore_ctx(&_ctx); \
    } while (0)
