
#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>


int vfs_init_global_context() {
    INIT_LIST_HEAD(&_laritos.fs.fstypes);
    INIT_LIST_HEAD(&_laritos.fs.mounts);

    INIT_LIST_HEAD(&_laritos.fs.root.children);
    INIT_LIST_HEAD(&_laritos.fs.root.siblings);
    strncpy(_laritos.fs.root.name, "/", sizeof(_laritos.fs.root.name));
    return 0;
}

static inline fs_type_t *get_fstype(char *fstype) {
    fs_type_t *fst;
    list_for_each_entry(fst, &_laritos.fs.fstypes, list) {
        if (strncmp(fst->id, fstype, strlen(fst->id)) == 0) {
            return fst;
        }
    }
    return NULL;
}

int vfs_register_fs_type(fs_type_t *fst) {
    if (get_fstype(fst->id) != NULL) {
        error("%s filesystem type already registered", fst->id);
        return -1;
    }

    if (fst->id == NULL || fst->id[0] == '\0') {
        error("File system type cannot have a null or empty id");
        return -1;
    }

    if (fst->mount == NULL) {
        error("File system type cannot have a null mount function");
        return -1;
    }

    debug("Registering FS type '%s'", fst->id);
    list_add_tail(&fst->list, &_laritos.fs.fstypes);
    return 0;
}

int vfs_unregister_fs_type(fs_type_t *fst) {
    if (fst == NULL) {
        return -1;
    }

    if (get_fstype(fst->id) == NULL) {
        error("%s filesystem type not registered", fst->id);
        return -1;
    }

    debug("Un-registering FS type '%s'", fst->id);
    list_del_init(&fst->list);
    return -1;
}

bool vfs_is_fs_type_supported(char *fstype) {
    return get_fstype(fstype) != NULL;
}

static inline fs_mount_t *get_fsmount(char *mount_point) {
    fs_mount_t *fsm;
    list_for_each_entry(fsm, &_laritos.fs.mounts, list) {
        if (strncmp(fsm->root.name, mount_point, sizeof(fsm->root.name)) == 0) {
            return fsm;
        }
    }
    return NULL;
}

bool vfs_is_fs_mounted(char *mount_point) {
    return get_fsmount(mount_point) != NULL;
}

void vfs_initialize_mount_struct(fs_mount_t *mount, fs_type_t *fstype, char *mount_point, uint16_t flags, void *params) {
    strncpy(mount->root.name, mount_point, sizeof(mount->root.name));
    mount->flags = flags;
    mount->sb->fstype = fstype;
    INIT_LIST_HEAD(&mount->list);
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params) {
    info("Mounting filesystem '%s' at %s with flags=0x%0x", fstype, mount_point, flags);

    fs_type_t *fst = get_fstype(fstype);
    if (fst == NULL) {
        error("File system '%s' not supported", fstype);
        return NULL;
    }

    if (vfs_is_fs_mounted(mount_point)) {
        error("Mount point '%s' already in use", mount_point);
        return NULL;
    }

    fs_mount_t *mount = fst->mount(fst, mount_point, flags, params);
    if (mount == NULL) {
        error("Could not mount file system '%s' at %s", fst->id, mount_point);
        return NULL;
    }

    if (mount->sb == NULL) {
        error("No superblock was instantiated");
        return NULL;
    }

    list_add_tail(&mount->list, &_laritos.fs.mounts);

    return mount;
}

int vfs_unmount_fs(char *mount_point) {
    fs_mount_t *fsm = get_fsmount(mount_point);
    if (fsm == NULL) {
        error("Cannot unmount: '%s' not mounted", mount_point);
        return -1;
    }

    info("Unmounting filesystem %s", mount_point);

    list_del_init(&fsm->list);
    if (fsm->ops.unmount(fsm) < 0) {
        error("Error while unmounting filesystem at %s", mount_point);
        return -1;
    }
    return 0;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_CORE
#include __FILE__
#endif
