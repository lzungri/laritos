#include <log.h>

#include <printf.h>

#include <arm.h>
#include <utils/utils.h>
#include "include/debug.h"

void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, psr_t cpsr) {
    psr_t v = cpsr;
    v.v = 1;

    log(false, "I", "Registers:");
    log(false, "I", "   pc=0x%08lx lr=0x%08lx cpsr=0x%08lx (%c%c%c%c%c%c%c%c %s mode)", pc, lr, cpsr.v,
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
            log(false, "I", "   %s", buf);
        }
    }
}

/**
 * Log arm registers
 *
 * Note: __attribute__((naked)) to avoid compiler prologue/epilogue, which may
 * change current register values
 */
__attribute__((naked)) void dump_all_regs(void) {
    // Save all relevant registers in the stack
    asm("stmfd sp!, {r0-r12, lr}");

    // 1st arg: Pointer to saved registers
    asm("mov r0, sp");
    // 2nd arg: # of registers
    asm("mov r1, #13");
    // 3rd arg: Program counter
    asm("sub r2, lr, #4");
    // 4th arg: Link register
    asm("mov r3, lr");
    // 5th arg (in the stack): Current PSR
    asm("mrs r4, cpsr");
    asm("push {r4}");
    asm("bl dump_regs");
    asm("pop {r4}");

    // Restore stack and continue execution in caller
    asm("ldmfd sp!, {r0-r12, pc}");
}
