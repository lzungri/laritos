#define DEBUG
#include <log.h>

#include <syscall/syscall.h>
#include <cpu.h>
#include <process/pcb.h>

#include <sched/core.h>

int syscall(int sysno, regsp_t sp, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    pcb_set_current_pcb_stack(sp);
    verbose_async("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);

    pcb_t *pcb = pcb_get_current();
    if (sysno == 0) {
        info_async("Killing process pid=%u", pcb->pid);
        sched_move_to_zombie(pcb);
        schedule();
    }
    return 0;
}
