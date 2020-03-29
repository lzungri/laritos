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
