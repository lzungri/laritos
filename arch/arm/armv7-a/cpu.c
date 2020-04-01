/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <cpu/core.h>
#include <core.h>
#include <component/intc.h>
#include <arch/cpu-types.h>
#include <irq/types.h>

static irqret_t pmu_irq_handler(irq_t irq, void *data) {
    verbose_async("PMU overflow interrupt");
    /**
     * From ARM ARM:
     * The PMOVSR holds the state of the overflow bit for:
     *    - the Cycle Count Register, PMCCNTR
     *    - each of the implemented event counters, PMNx
     *
     * C, bit[31]: PMCCNTR overflow bit. Write 1 to clear it
     *
     */
    uint32_t pmovsr;
    asm("mrc p15, 0, %0, c9, c12, 3" : "=r" (pmovsr));
    // Clear overflow bit
    pmovsr |= 1 << 31;
    asm("mcr p15, 0, %0, c9, c12, 3" : : "r" (pmovsr));
    // Increment by one the high-order part of the cycle counter
    _laritos.arch_data.high_cycle_counter++;

    return IRQ_RET_HANDLED;
}

int arch_cpu_set_cycle_count_enable(bool enable) {
    /**
     * From ARM ARM:
     * The PMCR provides details of the Performance Monitors implementation, including the
     * number of counters implemented, and configures and controls the counters.
     * This register is a Performance Monitors register.
     *
     * D, bit[3]: Cycle counter clock divider. The possible values of this bit are:
     *    - 0 When enabled, PMCCNTR counts every clock cycle.
     *    - 1 When enabled, PMCCNTR counts once every 64 clock cycles.
     * This bit is RW. Its non-debug logic reset value is 0.
     *
     * C, bit[2]: Cycle counter reset. This bit is WO. The effects of writing to this bit are:
     *    - 0 No action.
     *    - 1 Reset PMCCNTR to zero
     *
     * E, bit[0]: Enable. The possible values of this bit are:
     *    - 0 All counters, including PMCCNTR, are disabled.
     *    - 1 All counters are enabled.
     */
    uint32_t orig;
    asm("mrc p15, 0, %0, c9, c12, 0" : "=r" (orig));
    // Clear enable and D bits
    orig &= ~((uint32_t) 0b1001);
    // Set C bit
    orig |= 0b100;
    // Set/clear all counters enable bit based on <enable> argument
    orig |= enable ? 1 : 0;
    asm("mcr p15, 0, %0, c9, c12, 0" : : "r" (orig));

    if (!enable) {
        // Reset high-order part of the cycle counter
        _laritos.arch_data.high_cycle_counter = 0;
    }

    /**
     * From ARM ARM:
     * The PMCNTENSET register enables the Cycle Count Register, PMCCNTR, and any
     * implemented event counters, PMNx. Reading this register shows which counters are
     * enabled.
     * This register is a Performance Monitors register
     *
     * C, bit[31]: PMCCNTR enable bi
     */
    asm("mrc p15, 0, %0, c9, c12, 1" : "=r" (orig));
    // Clear enable bit
    orig &= ~(((uint32_t) 1) << 31);
    // Set/clear cycle counter enable bit based on <enable> argument
    orig |= enable ? 1 << 31 : 0;
    asm("mcr p15, 0, %0, c9, c12, 1" : : "r" (orig));

    // Add/remove PMU overflow interrupt handler
    arm_cpu_t *armcpu = (arm_cpu_t *) cpu();
    if (enable) {
        if (intc_enable_irq_with_handler(armcpu->parent.intc,
                armcpu->pmu_irq, armcpu->pmu_irq_trigger, pmu_irq_handler, NULL) < 0) {
            error("Failed to enable irq %u with handler 0x%p", armcpu->pmu_irq, pmu_irq_handler);
            return -1;
        }
    } else {
        intc_disable_irq_with_handler(armcpu->parent.intc, armcpu->pmu_irq, pmu_irq_handler);
    }

    /**
     * From ARM ARM:
     * The PMINTENSET register enables the generation of interrupt requests on overflows from:
     *    - the Cycle Count Register, PMCCNTR
     *    - each implemented event counter, PMNx.
     * Reading the register shows which overflow interrupt requests are enabled.
     * This register is a Performance Monitors register.
     *
     * C, bit[31]: PMCCNTR overflow interrupt request enable bit.
     */
    asm("mrc p15, 0, %0, c9, c14, 1" : "=r" (orig));
    // Clear enable bit
    orig &= ~(((uint32_t) 1) << 31);
    // Set/clear overflow enable bit based on <enable> argument
    orig |= enable ? 1 << 31 : 0;
    asm("mcr p15, 0, %0, c9, c14, 1" : : "r" (orig));
    return 0;
}

int arch_cpu_reset_cycle_count(void) {
    /**
     * From ARM ARM:
     * The PMCR provides details of the Performance Monitors implementation, including the
     * number of counters implemented, and configures and controls the counters.
     *
     * C, bit[2]: Cycle counter reset. This bit is WO. The effects of writing to this bit are:
     *    - 0 No action.
     *    - 1 Reset PMCCNTR to zero
     */
    uint32_t pmcr_orig;
    asm("mrc p15, 0, %0, c9, c12, 0" : "=r" (pmcr_orig));
    asm("mcr p15, 0, %0, c9, c12, 0" : : "r" (pmcr_orig | 0x4));
    return 0;
}

uint64_t arch_cpu_get_cycle_count(void) {
    /**
     * From ARM ARM:
     * The PMCCNTR holds the value of the processor Cycle Counter, CCNT, that counts
     * processor clock cycles.
     * This register is a Performance Monitors register.
     *
     * CCNT, bits[31:0] Cycle count. Depending on the value of the PMCR.D bit, this field increments either:
     *      - once every processor clock cycle
     *      - once every 64 processor clock cycles.
     */
    uint32_t v;
    asm("mrc p15, 0, %0, c9, c13, 0" : "=r" (v));
    return (((uint64_t) _laritos.arch_data.high_cycle_counter ) << 32) | v;
}
