#include <log.h>
#include <syscall32.h>
#include <arm.h>

int svc_handler(int sysno, spregs_t *regs) {
    syscall_params_t params = {
        .p0 = regs->r0,
        .p1 = regs->r1,
        .p2 = regs->r2,
        .p3 = regs->r3,
        .p4 = regs->r4,
        .p5 = regs->r5,
        .p6 = regs->r6,
    };
    return syscall(sysno, &params);
}

int undef_handler(void) {
    return 0;
}

int prefetch_handler(void) {
    return 0;
}

int abort_handler(void) {
    return 0;
}

int irq_handler(void) {
    return 0;
}

int fiq_handler(void) {
    return 0;
}
