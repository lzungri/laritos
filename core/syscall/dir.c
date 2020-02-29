#include <log.h>

#include <string.h>
#include <core.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <sync/spinlock.h>
#include <fs/file.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

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

int syscall_listdir(char *path, uint32_t offset, fs_listdir_t *dirs, int dirlen) {
    fs_file_t *f;
    if (path == NULL) {
        pcb_t *pcb = process_get_current();
        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
        f = vfs_file_dentry_open(pcb->cwd, FS_ACCESS_MODE_READ);
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    } else {
        f = vfs_file_open(path, FS_ACCESS_MODE_READ);
    }

    if (f == NULL) {
        error("No such directory");
        return -1;
    }

    int ret = vfs_dir_listdir(f, offset, dirs, dirlen);
    if (ret < 0) {
        error("Cannot list directory");
    }
    vfs_file_close(f);
    return ret;
}
