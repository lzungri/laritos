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
#include <arch/context-types.h>
#include <mm/exc-handlers.h>
#include <utils/math.h>

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

int _svc_handler(int sysno, const spctx_t *ctx) {
    return syscall(sysno, (spctx_t *) ctx, ctx->r[0], ctx->r[1], ctx->r[2], ctx->r[3], ctx->r[4], ctx->r[5]);
}

void _undef_handler(int32_t pc, spctx_t *ctx) {
    message_delimiter();
    error_async("Instruction 0x%08lx at 0x%08lx not recognized", *((uint32_t *) pc), pc);
    // cpsr is backed up in spsr during an exception
    dump_regs(ctx->r, ARRAYSIZE(ctx->r) - 1, pc, ctx->ret, ctx->spsr);

    uint32_t *ptr = (uint32_t *) max(pc - 4 * 8, 0);
    error_async("Instructions around pc=0x%p:", (void *) pc);
    int i;
    for (i = 0; i < 16; i++, ptr++) {
        error_async(" %s [0x%p] 0x%08lx", ptr == (uint32_t *) pc ? "->" : "  ", ptr, *ptr);
    }

    exc_undef_handler(pc, ctx);
}

void _prefetch_handler(int32_t pc, const ifsr_reg_t ifsr, spctx_t *ctx) {
    message_delimiter();
    char *fs = fault_status_msg[ifsr.b.fs_h << 4 | ifsr.b.fs_l];
    error_async("Instruction prefetch exception: %s", fs != NULL ? fs : "Unknown");
    // cpsr is backed up in spsr during an exception
    dump_regs(ctx->r, ARRAYSIZE(ctx->r) - 1, pc, ctx->ret, ctx->spsr);

    exc_prefetch_handler(pc, ctx);
}

void _abort_handler(int32_t pc, const dfsr_reg_t dfsr, spctx_t *ctx) {
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
    dump_regs(ctx->r, ARRAYSIZE(ctx->r) - 1, pc, ctx->ret, ctx->spsr);

    exc_abort_handler(pc, ctx);
}

int _irq_handler(spctx_t *ctx) {
    return irq_handler(ctx);
}

int _fiq_handler(void) {
    return 0;
}
