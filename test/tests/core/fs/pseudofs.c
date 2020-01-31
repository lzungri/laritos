#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs-core.h>
#include <fs/vfs-types.h>

static bool is_fs_mounted(char *mount_point) {
    return false;
}

T(pseudofs_fstype_is_supported_by_default) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(is_fs_mounted("/test"));
    vfs_unmount_fs("/test");
    tassert(!is_fs_mounted("/test"));
TEND

T(pseudofs_mount_populates_superblock) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(is_fs_mounted("/test"));
    tassert(fsm->sb != NULL);
    vfs_unmount_fs("/test");
    tassert(!is_fs_mounted("/test"));
TEND

T(pseudofs_) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(is_fs_mounted("/test"));


//    fs_inode_t inode = fsm->sb->ops.alloc_inode(fsm->sb);


    vfs_unmount_fs("/test");
    tassert(!is_fs_mounted("/test"));
TEND
