#include <log.h>

#include <cpu.h>
#include <process/core.h>
#include <sched/core.h>
#include <syscall/syscall.h>
#include <utils/assert.h>
#include <sched/context.h>
#include <time/time.h>
#include <mm/exc-handlers.h>

int syscall(int sysno, spctx_t *ctx, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    if (_laritos.process_mode) {
        process_set_current_pcb_stack_context(ctx);
    }

    // Cannot issue system calls while running in kernel mode, regardless of whether it comes from a process or
    // a non-process kernel routine
    if (arch_context_is_kernel(ctx)) {
        if (_laritos.process_mode) {
            error("Cannot issue a system call in a kernel process");
            handle_process_exception(process_get_current());
            // Execution will never reach this point
        } else {
            fatal("ABORT: Cannot issue a system call while in kernel mode");
        }
    }

    verbose_async("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);

    pcb_t *pcb = process_get_current();
    assert(pcb != NULL, "process_get_current() cannot be NULL on system call");

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
    case SYSCALL_SLEEP:
        ret = syscall_sleep((uint32_t) arg0);
        break;
    case SYSCALL_MSLEEP:
        ret = syscall_msleep((uint32_t) arg0);
        break;
    case SYSCALL_USLEEP:
        ret = syscall_usleep((uint32_t) arg0);
        break;
    case SYSCALL_SET_PRIORITY:
        ret = syscall_set_priority((uint8_t) arg0);
        break;
    case SYSCALL_SET_PROCESS_NAME:
        ret = syscall_set_process_name((char *) arg0);
        break;
    default:
        error_async("Unrecognized system call #%d", sysno);
        process_kill_and_schedule(pcb);
    }

    // About to finish the svc call, re-schedule if needed
    schedule_if_needed();

    return ret;
}
