#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>

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

    fs_type_t *fst = vfs_get_fstype(fstype);
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



#ifdef CONFIG_TEST_CORE_FS_VFS_MOUNT
#include __FILE__
#endif
