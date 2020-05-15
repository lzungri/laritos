#pragma once

#include <stdint.h>
#include <irq/types.h>
#include <irq/core.h>
#include <core.h>
#include <arch/cpu.h>
#include <cpu/cpu-local.h>
#include <component/cpu.h>
#include <component/component.h>
#include <assert.h>
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

static inline cpu_t *cpu(void) {
    cpu_t *c = CPU_LOCAL_GET(_laritos.cpu);
    assert(c != NULL, "cpu() cannot be NULL");
    return c;
}

static inline int cpu_initialize(void) {
    // Make sure we keep the same cpu during the entire function
    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);

    cpu_t *c = cpu();
    if (c->ops.set_irqs_enable(c, true) < 0) {
        error_async("Failed to enable irqs for cpu %u", c->id);
        irq_local_restore_ctx(&ctx);
        return -1;
    }

    if (c->ops.custom_initialization != NULL && c->ops.custom_initialization(c) < 0) {
        error_async("Failed to perform custom initialization for cpu %u", c->id);
        irq_local_restore_ctx(&ctx);
        return -1;
    }

    irq_local_restore_ctx(&ctx);
    return 0;
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
