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
#include <component/component.h>
#include <process/core.h>
#include <utils/utils.h>
#include <utils/conf.h>
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
    vfs_unmount_fs("/");
error_root:
    return -1;
}

int vfs_mount_from_config(void) {
    info("Mounting file systems from mount.conf");

    fs_file_t *f = vfs_file_open("/sys/conf/mount.conf", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open 'launch_on_boot.conf' file");
        return -1;
    }

    char mntpoint[CONFIG_FS_MAX_FILENAME_LEN];
    char devid[CONFIG_FS_MAX_FILENAME_LEN];
    char fstype[8];
    char perm[8];
    char *tokens[] = { mntpoint, devid, fstype, perm };
    uint32_t tokens_size[] = { sizeof(mntpoint), sizeof(devid), sizeof(fstype), sizeof(perm) };

    uint32_t offset = 0;
    int fret = 0;
    int ret;
    while ((ret = conf_readline(f, tokens, tokens_size, ARRAYSIZE(tokens), &offset)) != 0) {
        if (ret < 0) {
            continue;
        }

        fs_access_mode_t mode = 0;
        if (strchr(perm, 'r') != NULL) {
            mode |= FS_MOUNT_READ;
        }
        if (strchr(perm, 'w') != NULL) {
            mode |= FS_MOUNT_WRITE;
        }

        void *dev = component_get_by_id(devid);
        if (dev == NULL) {
            error("Couldn't find block device with id '%s' for mountpoint '%s'", devid, mntpoint);
            fret = -1;
            continue;
        }

        if (vfs_mount_fs(fstype, mntpoint, mode, (fs_param_t []) { { "dev", dev }, { NULL } }) < 0) {
            error("Could not mount %s", mntpoint);
            fret = -1;
        }
    }

    vfs_file_close(f);

    return fret;
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

void *vfs_get_param(fs_param_t *params, char *param) {
    if (params == NULL) {
        return NULL;
    }

    fs_param_t *p;
    for (p = params; p->param != NULL; p++) {
        if (strncmp(p->param, param, 32) == 0) {
            return p->value;
        }
    }
    return NULL;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_CORE
#include __FILE__
#endif
