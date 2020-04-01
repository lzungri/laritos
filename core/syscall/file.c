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

fs_file_t *syscall_open(char *path, fs_access_mode_t mode) {
    // Create file if it was opened for writing and it doesn't exit
    if ((mode & FS_ACCESS_MODE_WRITE) && !file_exist(path)) {
        fs_dentry_t *parent = vfs_dentry_lookup_parent(path);
        if (parent->inode->sb->fstype == vfs_get_fstype("pseudofs")) {
            error("Permission denied: Cannot create files on pseudo file systems");
            return NULL;
        }
        if (vfs_file_create(parent, file_get_basename(path), FS_ACCESS_MODE_READ | mode) == NULL) {
            error("Could not create file '%s'", path);
            return NULL;
        }
    }
    return vfs_file_open(path, mode);
}

int syscall_read(fs_file_t *file, void *buf, int buflen) {
    return vfs_file_read_cur_offset(file, buf, buflen);
}

int syscall_write(fs_file_t *file, void *buf, int buflen) {
    return vfs_file_write_cur_offset(file, buf, buflen);
}

int syscall_close(fs_file_t *file) {
    return vfs_file_close(file);
}
