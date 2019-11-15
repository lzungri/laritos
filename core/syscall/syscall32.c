#include <log.h>

#include <cpu.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <syscall/syscall.h>
#include <utils/assert.h>
#include <sched/context.h>

int syscall(int sysno, spctx_t *ctx, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT: Cannot issue a system call while in supervisor mode");
    }

    pcb_set_current_pcb_stack_context(ctx);
    verbose_async("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);

    pcb_t *pcb = pcb_get_current();
    assert(pcb != NULL, "pcb_get_current() cannot be NULL on system call");

    switch (sysno) {
    case 0:
        info_async("Killing process pid=%u", pcb->pid);
        pcb_kill(pcb);
        schedule();
        break;
    case 1:
        info_async("Yielding process pid=%u", pcb->pid);
        schedule();
        break;
    case 2:
        info_async("[pid=%u] %s", pcb->pid, (char *) arg0);
        break;
    }
    return 0;
}
