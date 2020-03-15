#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <strtoxl.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <fs/pseudofs.h>
#include <mm/heap.h>
#include <process/core.h>
#include <utils/utils.h>
#include <generated/autoconf.h>


int vfs_init_global_context() {
    INIT_LIST_HEAD(&_laritos.fs.fstypes);
    INIT_LIST_HEAD(&_laritos.fs.mounts);
    INIT_LIST_HEAD(&_laritos.fs.sysfs_mods);
    return 0;
}

fs_type_t *vfs_get_fstype(char *fstype) {
    fs_type_t *fst;
    list_for_each_entry(fst, &_laritos.fs.fstypes, list) {
        if (strncmp(fst->id, fstype, strlen(fst->id)) == 0) {
            return fst;
        }
    }
    return NULL;
}

int vfs_register_fs_type(fs_type_t *fst) {
    if (vfs_get_fstype(fst->id) != NULL) {
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

    if (vfs_get_fstype(fst->id) == NULL) {
        error("%s filesystem type not registered", fst->id);
        return -1;
    }

    debug("Un-registering FS type '%s'", fst->id);
    list_del_init(&fst->list);
    return -1;
}

bool vfs_is_fs_type_supported(char *fstype) {
    return vfs_get_fstype(fstype) != NULL;
}

int vfs_mount_essential_filesystems(void) {
    info("Mounting root filesystem");
    fs_mount_t *mnt = vfs_mount_fs("pseudofs", "/", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting root filesystem");
        goto error_root;
    }
    _laritos.fs.root = mnt->root;

    info("Mounting system filesystem");
    mnt = vfs_mount_fs("ext2", "/sys", FS_MOUNT_READ | FS_MOUNT_WRITE,
            (fs_param_t []) {
                { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_BASE) },
                { NULL },
            });
    if (mnt == NULL) {
        error("Error mounting data");
        goto error_data;
    }
    _laritos.fs.sys_root = mnt->root;

    _laritos.fs.stats_root = vfs_dir_create(_laritos.fs.root, "stats",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.stats_root == NULL) {
        error("Error creating stats sysfs directory");
        goto error_stats;
    }

    fs_sysfs_mod_t *sysfs;
    list_for_each_entry(sysfs, &_laritos.fs.sysfs_mods, list) {
        debug("Creating sysfs files for '%s'", sysfs->id);
        if (sysfs->create(sysfs) < 0) {
            error("Failed to create sysfs files for '%s'", sysfs->id);
            break;
        }
    }

    // Check if there was an error creating the sysfs nodes
    if (&sysfs->list != &_laritos.fs.sysfs_mods) {
        list_for_each_entry_continue_reverse(sysfs, &_laritos.fs.sysfs_mods, list) {
            error("Removing sysfs files for '%s'", sysfs->id);
            if (sysfs->remove(sysfs) < 0) {
                error("Failed to remove sysfs files for '%s'", sysfs->id);
                break;
            }
        }
        goto error_sysfs_mods;
    }

    return 0;

error_sysfs_mods:
    vfs_dir_remove(_laritos.fs.root, "stats");
error_stats:
    vfs_unmount_fs("/sys");
error_data:
    vfs_unmount_fs("/");
error_root:
    return -1;
}

int vfs_register_sysfs(fs_sysfs_mod_t *sysfs) {
    if (sysfs->id == NULL || sysfs->id[0] == '\0') {
        error("Sysfs module cannot have a null or empty id");
        return -1;
    }

    if (sysfs->create == NULL || sysfs->remove == NULL) {
        error("One or more mandatory sysfs module functions are missing");
        return -1;
    }

    debug("Registering sysfs module '%s'", sysfs->id);
    list_add_tail(&sysfs->list, &_laritos.fs.sysfs_mods);

    return 0;
}

int vfs_unregister_sysfs(fs_sysfs_mod_t *sysfs) {
    if (sysfs == NULL) {
        return -1;
    }

    debug("Un-registering sysfs module '%s'", sysfs->id);
    list_del_init(&sysfs->list);

    return 0;
}

int vfs_get_param_uint32(fs_param_t *params, char *param, uint32_t *value, uint8_t base) {
    if (params == NULL) {
        return -1;
    }

    fs_param_t *p;
    for (p = params; p->param != NULL; p++) {
        if (strncmp(p->param, param, 32) == 0) {
            *value = (uint32_t) strtoul(p->value, NULL, base);
            return 0;
        }
    }
    return -1;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_CORE
#include __FILE__
#endif
