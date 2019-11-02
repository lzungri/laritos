#pragma once

#include <stdint.h>
#include <cpu.h>

/**
 * @return: Processor mode string for the given <mode>
 */
static inline const char *get_cpu_mode_str(uint8_t mode) {
    static const char modes[16][4] = {
        "usr", "fiq", "irq", "svc", "???", "???", "mon", "abt",
        "???", "???", "hyp", "und", "???", "???", "???", "sys"
    };
    return modes[mode & 0xf];
}

void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, regpsr_t cpsr);

/**
 * Log arm registers
 *
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful PC/LR, not just the PC inside the arch_dump_all_regs() function (in case it
 * wasn't expanded by the compiler)
 */
__attribute__((always_inline)) static inline void arch_dump_all_regs(void) {
    // Save all relevant registers in the stack
    asm("stmfd sp!, {r0-r12, lr}");

    // 1st arg: Pointer to saved registers
    asm("mov r0, sp");
    // 2nd arg: # of registers
    asm("mov r1, #13");
    // 3rd arg: Program counter (SUB to climb up to the function caller)
    asm("sub r2, pc, #20");
    // 4th arg: Link register
    asm("mov r3, lr");
    // 5th arg (in the stack): Current PSR
    asm("mrs r4, cpsr");
    asm("push {r4}");
    asm("bl dump_regs");
    asm("pop {r4}");

    // Restore stack
    asm("ldmfd sp!, {r0-r12, lr}");
}
