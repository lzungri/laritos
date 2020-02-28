#include <log.h>

#include <string.h>
#include <core.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <sync/spinlock.h>
#include <utils/file.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

fs_file_t *syscall_open(char *path, fs_access_mode_t mode) {
    return vfs_file_open(path, mode);
}

int syscall_read(fs_file_t *file, void *buf, int buflen) {
    return vfs_file_read_cur_offset(file, buf, buflen);
}

int syscall_close(fs_file_t *file) {
    return vfs_file_close(file);
}
