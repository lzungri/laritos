#include <log.h>

#include <fs/vfs-types.h>
#include <fs/vfs-core.h>

int vfs_register_fs_type(fs_type_t *fst) {
    return 0;
}

int vfs_unregister_fs_type(fs_type_t *fst) {
    return 0;
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params) {
    return NULL;
}

int vfs_unmount_fs(char *mount_point) {
    return 0;
}



#ifdef CONFIG_TEST_CORE_FS_VFSCORE
#include __FILE__
#endif
