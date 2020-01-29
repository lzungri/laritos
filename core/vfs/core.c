#include <log.h>

#include <vfs/core.h>
#include <vfs/types.h>


int vfs_register_fs_type(fs_type_t *fst) {
    return 0;
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params) {
    return NULL;
}


#ifdef CONFIG_TEST_CORE_VFS_CORE
#include __FILE__
#endif
