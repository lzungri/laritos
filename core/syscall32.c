#define DEBUG
#include <log.h>
#include <syscall32.h>

int syscall(int sysno, syscall_params_t *params) {
    // TODO Use verbose_async
    verbose("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx, %lx)", sysno,
            params->p0, params->p1, params->p2, params->p3,
            params->p4, params->p5, params->p6);
    return 0;
}
