#define DEBUG
#include <log.h>
#include <syscall32.h>

int syscall(int sysno, syscall_params_t *params) {
    // TODO Use verbose_async
    verbose("syscall_%d(%lx, %lx, %lx, %lx, %lx, %lx)", sysno,
            params->p[0], params->p[1], params->p[2], params->p[3],
            params->p[4], params->p[5]);
    return 0;
}
