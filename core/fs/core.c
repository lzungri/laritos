#include <log.h>

#include <core.h>
#include <strtoxl.h>
#include <process/core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>
#include <dstruct/list.h>
#include <utils/utils.h>
#include <generated/autoconf.h>

int fs_mount_essential_filesystems(void) {
    info("Mounting root filesystem");
    fs_mount_t *mnt = vfs_mount_fs("pseudofs", "/", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting root filesystem");
        goto error_root;
    }
    _laritos.fs.root = mnt->root;

    info("Mounting kernel pseudo filesystem");
    mnt = vfs_mount_fs("pseudofs", "/kernel", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting sysfs");
        goto error_sysfs;
    }
    _laritos.fs.kernelfs_root = mnt->root;

    info("Mounting system filesystem");
    mnt = vfs_mount_fs("ext2", "/sys", FS_MOUNT_READ | FS_MOUNT_WRITE,
            (fs_param_t []) {
                { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
                { NULL },
            });
    if (mnt == NULL) {
        error("Error mounting data");
        goto error_data;
    }
    _laritos.fs.sys_root = mnt->root;

    _laritos.fs.stats_root = vfs_dir_create(_laritos.fs.kernelfs_root, "stats",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.stats_root == NULL) {
        error("Error creating stats sysfs directory");
        goto error_stats;
    }

    sysfs_mod_t *sysfs;
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
    vfs_dir_remove(_laritos.fs.kernelfs_root, "stats");
error_stats:
    vfs_unmount_fs("/data");
error_data:
    vfs_unmount_fs("/sys");
error_sysfs:
    vfs_unmount_fs("/");
error_root:
    return -1;
}

int fs_register_sysfs(sysfs_mod_t *sysfs) {
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

int fs_unregister_sysfs(sysfs_mod_t *sysfs) {
    if (sysfs == NULL) {
        return -1;
    }

    debug("Un-registering sysfs module '%s'", sysfs->id);
    list_del_init(&sysfs->list);

    return 0;
}

int fs_get_param_uint32(fs_param_t *params, char *param, uint32_t *value) {
    if (params == NULL) {
        return -1;
    }

    fs_param_t *p;
    for (p = params; p->param != NULL; p++) {
        if (strncmp(p->param, param, 32) == 0) {
            *value = (uint32_t) strtoul(p->value, NULL, 0);
            return 0;
        }
    }
    return -1;
}
