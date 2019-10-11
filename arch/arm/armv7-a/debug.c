#include <log.h>

#include <printf.h>

#include <utils/utils.h>
#include <arch/cpu.h>
#include <arch/debug.h>

void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, psr_t cpsr) {
    psr_t v = cpsr;
    v.v = 1;

    log_always_async("Registers:");
    log_always_async("   pc=0x%08lx lr=0x%08lx cpsr=0x%08lx (%c%c%c%c%c%c%c%c %s mode)", pc, lr, cpsr.v,
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
            log_always_async("   %s", buf);
        }
    }
}
