#pragma once

#include <stdint.h>
#include <core.h>
#include <arch/cpu.h>
#include <component/cpu.h>
#include <component/component.h>
#include <utils/assert.h>

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
