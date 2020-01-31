#define DEBUG
#include <log.h>

#include <core.h>
#include <fs/vfs-types.h>
#include <fs/vfs-core.h>


int vfs_init_global_context() {
    INIT_LIST_HEAD(&_laritos.fs.fstypes);
    return 0;
}

int vfs_register_fs_type(fs_type_t *fst) {
    debug("Registering FS type '%s'", fst->id);
    list_add_tail(&fst->list, &_laritos.fs.fstypes);
    return 0;
}

int vfs_unregister_fs_type(fs_type_t *fst) {
    debug("Un-registering FS type '%s'", fst->id);
    list_del_init(&fst->list);
    return -1;
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params) {
    info("Mounting filesystem '%s' at %s with flags=0x%0x", fstype, mount_point, flags);
    return NULL;
}

int vfs_unmount_fs(char *mount_point) {
    info("Unmounting filesystem %s", mount_point);
    return -1;
}



#ifdef CONFIG_TEST_CORE_FS_VFSCORE
#include __FILE__
#endif
