#include <log.h>
#include <printf.h>
#include <core.h>
#include <component/component.h>
#include <component/intc.h>
#include <irq.h>
#include <generated/autoconf.h>
#include <syscall/syscall.h>
#include <utils/utils.h>
#include <utils/debug.h>
#include <arch/debug.h>
#include <arch/cpu.h>
#include <arch/stack.h>

/**
 * Fault messages according to the armv7-a ARM document
 */
static char *fault_status_msg[32] = {
    [0b00001] = "Alignment fault",
    [0b00100] = "Fault on instruction cache maintenance",
    [0b01100] = "Synchronous external abort on translation table walk (first level)",
    [0b01110] = "Synchronous external abort on translation table walk (second level)",
    [0b11100] = "Synchronous parity error on translation table walk (first level)",
    [0b11110] = "Synchronous parity error on translation table walk (second level)",
    [0b00101] = "Translation fault (first level)",
    [0b00111] = "Translation fault (second level)",
    [0b00011] = "Access flag fault (first level)",
    [0b00110] = "Access flag fault (second level)",
    [0b01001] = "Domain fault (first level)",
    [0b01011] = "Domain fault (second level)",
    [0b01101] = "Permission fault (first level)",
    [0b01111] = "Permission fault (second level)",
    [0b00010] = "Debug event",
    [0b01000] = "Synchronous external abort",
    [0b10000] = "TLB conflict abort",
    [0b10100] = "IMPLEMENTATION DEFINED",
    [0b11010] = "IMPLEMENTATION DEFINED",
    [0b11001] = "Synchronous parity error on memory access",
    [0b10110] = "Asynchronous external abort",
    [0b11000] = "Asynchronous parity error on memory access",
};

int _svc_handler(int sysno, const spregs_t *regs) {
    return syscall(sysno, (regsp_t) regs, regs->r[0], regs->r[1], regs->r[2], regs->r[3], regs->r[4], regs->r[5]);
}

int _undef_handler(int32_t pc, const spregs_t *regs) {
    message_delimiter();
    error_async("Instruction 0x%08lx at 0x%08lx not recognized", *((uint32_t *) pc), pc);
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->ret, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int _prefetch_handler(int32_t pc, const ifsr_reg_t ifsr, const spregs_t *regs) {
    message_delimiter();
    char *fs = fault_status_msg[ifsr.b.fs_h << 4 | ifsr.b.fs_l];
    error_async("Instruction prefetch exception: %s", fs != NULL ? fs : "Unknown");
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->ret, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int _abort_handler(int32_t pc, const dfsr_reg_t dfsr, const spregs_t *regs) {
    /**
     * From ARM ARM document:
     * After taking a Data Abort exception, the state of the exclusive monitors is UNKNOWN. Therefore,
     * ARM strongly recommends that the abort handler performs a CLREX instruction, or a dummy STREX
     * instruction, to clear the exclusive monitor state.
     */
    asm("clrex");

    message_delimiter();
    char *fs = fault_status_msg[dfsr.b.fs_h << 4 | dfsr.b.fs_l];
    error_async("Data abort exception. Invalid %s access: %s", dfsr.b.wnr ? "write" : "read", fs != NULL ? fs : "Unknown");
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->ret, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int _irq_handler(const spregs_t *regs) {
    component_t *c = NULL;
    for_each_component_type(c, COMP_TYPE_INTC) {
        verbose_async("Dispatching irq to int controller '%s'", c->id);
        intc_t *intc = (intc_t *) c;
        irqret_t ret = intc->ops.dispatch_irq(intc);
        verbose_async("Interrupt controller '%s' returned %s", c->id, get_irqret_str(ret));
        switch (ret) {
        case IRQ_RET_HANDLED:
            return 0;
        case IRQ_RET_ERROR:
            error_async("Error while dispatching irq with intc '%s'", c->id);
            return -1;
        case IRQ_RET_NOT_HANDLED:
        case IRQ_RET_HANDLED_KEEP_PROCESSING:
        default:
            break;
        }
    }
    return 0;
}

int _fiq_handler(void) {
    return 0;
}
