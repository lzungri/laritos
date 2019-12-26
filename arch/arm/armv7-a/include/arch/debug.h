#pragma once

#include <stdint.h>
#include <printf.h>
#include <cpu.h>
#include <arch/context-types.h>
#include <printf.h>

/**
 * @return: Processor mode string for the given <mode>
 */
static inline const char *arch_get_cpu_mode_str(uint8_t mode) {
    static const char modes[16][4] = {
        "usr", "fiq", "irq", "svc", "???", "???", "mon", "abt",
        "???", "???", "hyp", "und", "???", "???", "???", "sys"
    };
    return modes[mode & 0xf];
}

static inline char *arch_get_psr_str(regpsr_t psr, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%c%c%c%c%c%c%c%c %s", psr.b.n ? 'N' : '.', psr.b.z ? 'Z' : '.', psr.b.c ? 'C' : '.',
            psr.b.v ? 'V' : '.', psr.b.q ? 'Q' : '.', psr.b.async_abort ? '.' : 'A',
            psr.b.irq ? '.' : 'I', psr.b.fiq ? '.' : 'F', arch_get_cpu_mode_str(psr.b.mode));
    return buf;
}

static inline char *arch_get_psr_str_from_ctx(spctx_t *ctx, char *buf, size_t buflen) {
    return arch_get_psr_str(ctx->spsr, buf, buflen);
}

static inline void arch_dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, regpsr_t cpsr) {
    regpsr_t v = cpsr;
    v.v = 1;

    char cpsrbuf[64] = { 0 };
    error_async("Registers:");
    error_async("   pc=0x%08lx lr=0x%08lx cpsr=0x%08lx (%s mode)", pc, lr, cpsr.v, arch_get_psr_str(cpsr, cpsrbuf, sizeof(cpsrbuf)));

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
    void (*f)(void) = (void (*)(void)) arch_dump_regs;
    f();
    asm("pop {r4}");

    // Restore stack
    asm("ldmfd sp!, {r0-r12, lr}");
}
