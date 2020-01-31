#define DEBUG
#include <log.h>

#include <core.h>
#include <string.h>
#include <dstruct/list.h>
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

static inline fs_type_t *get_fstype(char *fstype) {
    fs_type_t *fst;
    list_for_each_entry(fst, &_laritos.fs.fstypes, list) {
        if (strncmp(fst->id, fstype, strlen(fstype)) == 0) {
            return fst;
        }
    }
    return NULL;
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params) {
    info("Mounting filesystem '%s' at %s with flags=0x%0x", fstype, mount_point, flags);

    fs_type_t *fst = get_fstype(fstype);
    if (fst == NULL) {
        error("File system '%s' not supported", fstype);
        return NULL;
    }

    fs_mount_t *mount = fst->mount(fst, mount_point, flags, params);
    if (mount == NULL) {
        error("Could not mount file system '%s' at %s", fst->id, mount_point);
        return NULL;
    }

    strncpy(mount->mount_point, mount_point, sizeof(mount->mount_point));
    mount->flags = flags;
    mount->sb->fstype = fstype;

    return mount;
}

int vfs_unmount_fs(char *mount_point) {
    info("Unmounting filesystem %s", mount_point);
    return -1;
}



#ifdef CONFIG_TEST_CORE_FS_VFSCORE
#include __FILE__
#endif
