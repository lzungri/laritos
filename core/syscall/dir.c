#include <log.h>

#include <string.h>
#include <core.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <sync/spinlock.h>
#include <utils/file.h>
#include <fs/vfs/core.h>

int syscall_getcwd(char *buf, int buflen) {
    pcb_t *pcb = process_get_current();
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    vfs_dentry_get_fullpath(pcb->cwd, buf, buflen);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return 0;
}

int syscall_chdir(char *path) {
    if (!file_is_dir(path)) {
        error("%s: No such directory", path);
        return -1;
    }

    pcb_t *pcb = process_get_current();
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    pcb->cwd = vfs_dentry_lookup(path);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return 0;
}
