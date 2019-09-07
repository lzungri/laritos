#include <log.h>
#include <syscall.h>
#include <arm.h>
#include <utils.h>
#include <printf.h>
#include <debug.h>
#include <generated/autoconf.h>

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

static void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, int32_t cpsr) {
    log(false, "I", "Registers:");
    log(false, "I", "   pc=0x%08lx lr=0x%08lx cpsr=0x%08lx", pc, lr, cpsr);
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

int svc_handler(int sysno, const spregs_t *regs) {
    syscall_params_t params = { 0 };
    int i;
    for (i = 0; i < CONFIG_SYSCALL_MAX_ARGS && i < ARRAYSIZE(regs->r); i++) {
        params.p[i] = regs->r[i];
    }
    return syscall(sysno, &params);
}

int undef_handler(int32_t pc, const spregs_t *regs) {
    message_delimiter();
    error_sync(false, "Instruction 0x%08lx at 0x%08lx not recognized", *((uint32_t *) pc), pc);
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->lr, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int prefetch_handler(int32_t pc, const ifsr_reg_t ifsr, const spregs_t *regs) {
    message_delimiter();
    char *fs = fault_status_msg[ifsr.b.fs_h << 4 | ifsr.b.fs_l];
    error_sync(false, "Instruction prefetch exception: %s", fs != NULL ? fs : "Unknown");
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->lr, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int abort_handler(int32_t pc, const dfsr_reg_t dfsr, const spregs_t *regs) {
    message_delimiter();
    char *fs = fault_status_msg[dfsr.b.fs_h << 4 | dfsr.b.fs_l];
    error_sync(false, "Data abort exception. Invalid %s access: %s", dfsr.b.wnr ? "write" : "read", fs != NULL ? fs : "Unknown");
    // cpsr is backed up in spsr during an exception
    dump_regs(regs->r, ARRAYSIZE(regs->r), pc, regs->lr, get_spsr());
    message_delimiter();

    fatal("ABORT");
    return 0;
}

int irq_handler(void) {
    return 0;
}

int fiq_handler(void) {
    return 0;
}
