#define DEBUG
#include <log.h>

#include <cpu.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <syscall/syscall.h>
#include <utils/assert.h>

int syscall(int sysno, regsp_t sp, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    pcb_set_current_pcb_stack(sp);
    verbose_async("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);

    pcb_t *pcb = pcb_get_current();
    assert(pcb != NULL, "pcb_get_current() cannot be NULL on system call");

    switch (sysno) {
    case 0:
        info_async("Killing process pid=%u", pcb->pid);
        sched_move_to_zombie(pcb);
        schedule();
        break;
    case 1:
        info_async("Yielding process pid=%u", pcb->pid);
        schedule();
        break;
    }
    return 0;
}
