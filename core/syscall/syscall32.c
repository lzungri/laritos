#include <log.h>

#include <cpu.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <syscall/syscall.h>
#include <utils/assert.h>
#include <sched/context.h>
#include <time/time.h>

int syscall(int sysno, spctx_t *ctx, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    if (arch_context_is_kernel(ctx)) {
        fatal("ABORT: Cannot issue a system call while in supervisor mode");
    }

    pcb_set_current_pcb_stack_context(ctx);
    verbose_async("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);

    pcb_t *pcb = pcb_get_current();
    assert(pcb != NULL, "pcb_get_current() cannot be NULL on system call");

    int ret = 0;
    switch (sysno) {
    case SYSCALL_EXIT:
        syscall_exit((int) arg0);
        break;
    case SYSCALL_YIELD:
        ret = syscall_yield();
        break;
    case SYSCALL_PUTS:
        ret = syscall_puts((const char *) arg0);
        break;
    case SYSCALL_GETPID:
        ret = syscall_getpid();
        break;
    case SYSCALL_TIME:
        ret = syscall_time((time_t *) arg0);
        break;
    default:
        error_async("Unrecognized system call #%d", sysno);
        pcb_kill_and_schedule(pcb);
    }
    return ret;
}
