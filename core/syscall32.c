#define DEBUG
#include <log.h>
#include <syscall32.h>

int syscall(int sysno, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    verbose_sync(false, "syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno, arg0, arg1, arg2, arg3, arg4, arg5);
    return 0;
}
