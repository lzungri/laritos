#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

T(pseudofs_fstype_is_supported_by_default) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    vfs_unmount_fs("/test");
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_mount_populates_superblock) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    tassert(fsm->sb != NULL);
    vfs_unmount_fs("/test");
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    vfs_unmount_fs("/test");
    tassert(!vfs_dentry_exist("/test"));
TEND
