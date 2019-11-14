#pragma once

#include <stdint.h>
#include <printf.h>
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

static inline void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, regpsr_t cpsr) {
    regpsr_t v = cpsr;
    v.v = 1;

    error_async("Registers:");
    error_async("   pc=0x%08lx lr=0x%08lx cpsr=0x%08lx (%c%c%c%c%c%c%c%c %s mode)", pc, lr, cpsr.v,
            cpsr.b.n ? 'N' : '.', cpsr.b.z ? 'Z' : '.', cpsr.b.c ? 'C' : '.',
            cpsr.b.v ? 'V' : '.', cpsr.b.q ? 'Q' : '.', cpsr.b.async_abort ? '.' : 'A',
            cpsr.b.irq ? '.' : 'I', cpsr.b.fiq ? '.' : 'F', get_cpu_mode_str(cpsr.b.mode));
    int i;
    char buf[128] = { 0 };
    int written = 0;
    for (i = 0; i < nregs; i++) {
        written += snprintf(buf + written, sizeof(buf) - written, "r%u=0x%08lx ", i, regs[i]);
        if ((i + 1) % 4 == 0 || i == nregs - 1) {
            written = 0;
            error_async("   %s", buf);
        }
    }
}

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
    void (*f)(void) = (void (*)(void)) dump_regs;
    f();
    asm("pop {r4}");

    // Restore stack
    asm("ldmfd sp!, {r0-r12, lr}");
}
