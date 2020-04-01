/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

int syscall_mkdir(char *path, fs_access_mode_t mode) {
    fs_dentry_t *parent = vfs_dentry_lookup_parent(path);
    if (parent->inode->sb->fstype == vfs_get_fstype("pseudofs")) {
        error("Permission denied: Cannot create directories on pseudo file systems");
        return -1;
    }
    return vfs_dir_create(parent, file_get_basename(path), mode) != NULL ? 0 : -1;
}
