#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/ext2.h>
#include <fs/file.h>

T(ext2_mount_adds_a_new_fs_under_mount_point) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);
    tassert(fsm->flags == (FS_MOUNT_READ | FS_MOUNT_WRITE));
    tassert(strncmp(fsm->root->name, "test", sizeof(fsm->root->name)) == 0);
    tassert(file_is_dir("/test"));

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND
