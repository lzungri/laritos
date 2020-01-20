#pragma once

#include <log.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <component/cpu.h>

typedef struct {
    cpu_t parent;

    /**
     * IRQ used by the PMU (Performance Monitor Unit) to signal PMCCNTR
     * (cycle counter register) overflows
     */
    irq_t pmu_irq;

    /**
     * Trigger mode for the PMU irq
     */
    irq_trigger_mode_t pmu_irq_trigger;
} arm_cpu_t;
