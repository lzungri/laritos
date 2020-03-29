#include <log.h>

#include <stdbool.h>
#include <cpu/core.h>
#include <process/core.h>
#include <sched/core.h>
#include <syscall/syscall.h>
#include <utils/assert.h>
#include <utils/utils.h>
#include <sched/context.h>
#include <time/core.h>
#include <sync/atomic.h>
#include <mm/exc-handlers.h>
#include <generated/autoconf.h>


#define DEF_SCE(_sysno, _func) \
    [_sysno] = { (int (*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t)) _func, TOSTRING(_func) }

typedef struct {
    int (*call)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5);
    char *scname;
} syscall_entry_t;

static syscall_entry_t systable[] = {
    DEF_SCE(SYSCALL_EXIT, syscall_exit),
    DEF_SCE(SYSCALL_YIELD, syscall_yield),
    DEF_SCE(SYSCALL_PUTS, syscall_puts),
    DEF_SCE(SYSCALL_GETPID, syscall_getpid),
    DEF_SCE(SYSCALL_TIME, syscall_time),
    DEF_SCE(SYSCALL_SLEEP, syscall_sleep),
    DEF_SCE(SYSCALL_MSLEEP, syscall_msleep),
    DEF_SCE(SYSCALL_USLEEP, syscall_usleep),
    DEF_SCE(SYSCALL_SET_PRIORITY, syscall_set_priority),
    DEF_SCE(SYSCALL_SET_PROCESS_NAME, syscall_set_process_name),
    DEF_SCE(SYSCALL_READLINE, syscall_readline),
    DEF_SCE(SYSCALL_GETC, syscall_getc),
#ifdef CONFIG_SYSCALL_OPEN_BACKDOOR
    DEF_SCE(SYSCALL_BACKDOOR, syscall_backdoor),
#endif
    DEF_SCE(SYSCALL_CHDIR, syscall_chdir),
    DEF_SCE(SYSCALL_GETCWD, syscall_getcwd),
    DEF_SCE(SYSCALL_LISTDIR, syscall_listdir),
    DEF_SCE(SYSCALL_OPEN, syscall_open),
    DEF_SCE(SYSCALL_READ, syscall_read),
    DEF_SCE(SYSCALL_WRITE, syscall_write),
    DEF_SCE(SYSCALL_CLOSE, syscall_close),
    DEF_SCE(SYSCALL_GET_PROPERTY, syscall_get_property),
    DEF_SCE(SYSCALL_SPAWN_PROCESS, syscall_spawn_process),
    DEF_SCE(SYSCALL_WAITPID, syscall_waitpid),
    DEF_SCE(SYSCALL_MKDIR, syscall_mkdir),
};


int syscall(int sysno, spctx_t *ctx, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    if (_laritos.process_mode) {
        process_set_current_pcb_stack_context(ctx);
    }

    // Cannot issue system calls while running in kernel mode, regardless of whether it comes from a process or
    // a non-process kernel routine
    if (arch_context_is_kernel(ctx)) {
        if (_laritos.process_mode) {
            error("Cannot issue a system call in a kernel process");
            exc_handle_generic_process_error(process_get_current(), ctx);
            // Execution will never reach this point
        } else {
            fatal("ABORT: Cannot issue a system call while in kernel mode");
        }
    }

    pcb_t *pcb = process_get_current();
    assert(pcb != NULL, "process_get_current() cannot be NULL on system call");

    if (sysno >= ARRAYSIZE(systable) || systable[sysno].call == NULL) {
        error_async("Unrecognized or unsupported system call #%d", sysno);
        exc_handle_generic_process_error(pcb, ctx);
        // Execution will never reach this point
    }

    // Update process system calls stats
    atomic32_inc(&pcb->stats.syscalls[sysno]);

    // Enable interrupts while processing system calls
    irq_enable_local();

    verbose_async("%s(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)", systable[sysno].scname, arg0, arg1, arg2, arg3, arg4, arg5);
    int ret = systable[sysno].call(arg0, arg1, arg2, arg3, arg4, arg5);

    irq_disable_local();

    // About to finish the svc call, re-schedule if needed
    schedule_if_needed();

    return ret;
}
