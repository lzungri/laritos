#pragma once

#include <stdint.h>
#include <core.h>
#include <irq.h>
#include <arch/cpu.h>
#include <component/cpu.h>
#include <component/component.h>
#include <utils/assert.h>
#include <generated/autoconf.h>

typedef uint32_t cpubits_t;

#define CPU_ALL_MASK ((cpubits_t) -1)
#define BIT_FOR_CPU(_n) ((cpubits_t) (1 << _n))

typedef enum {
    CPU_MODE_SUPERVISOR = 0,
    CPU_MODE_USER,
} cpu_mode_t;


static inline uint8_t cpu_get_id(void) {
    return arch_cpu_get_id();
}

#define DEF_CPU_LOCAL(_type, _name) \
    __typeof__(_type) _name[CONFIG_CPU_MAX_CPUS]

#define CPU_LOCAL_GET(_name) \
    (({                                    \
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

// TODO: Optimize this
// Use CPU_LOCAL?
static inline cpu_t *cpu(void) {
    uint8_t cpuid = cpu_get_id();
    component_t *c;
    for_each_component_type(c, COMP_TYPE_CPU) {
        cpu_t *cpu = (cpu_t *) c;
        if (cpu->id == cpuid) {
            return cpu;
        }
    }
    assert(false, "cpu() cannot be NULL");
    return NULL;
}

/**
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful PC, not just the PC inside the regs_get_pc() function (in case it
 * wasn't expanded by the compiler)
 *
 * @return Current program counter
 */
__attribute__((always_inline)) static inline regpc_t cpu_get_pc(void) {
    return arch_cpu_get_pc();
}

/**
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful return value, not just the return address of the regs_get_pc() function (in case it
 * wasn't expanded by the compiler)
 *
 * @return: Function return address
 */
__attribute__((always_inline)) static inline regpc_t cpu_get_retaddr(void) {
    return arch_cpu_get_retaddr();
}

static inline int cpu_set_cycle_count_enable(bool enable) {
    return arch_cpu_set_cycle_count_enable(enable);
}

static inline int cpu_reset_cycle_count(void) {
    return arch_cpu_reset_cycle_count();
}

static inline uint64_t cpu_get_cycle_count(void) {
    return arch_cpu_get_cycle_count();
}
