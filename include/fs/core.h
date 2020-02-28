#pragma once

typedef struct sysfs_mod {
    char *id;
    list_head_t list;
    int (*create)(struct sysfs_mod *sysfs);
    int (*remove)(struct sysfs_mod *sysfs);
} sysfs_mod_t;


int fs_mount_essential_filesystems(void);
int fs_register_sysfs(sysfs_mod_t *sysfs);
int fs_unregister_sysfs(sysfs_mod_t *sysfs);

#define SYSFS_MODULE(_id, _create_sysfs, _remove_sysfs) \
    static sysfs_mod_t _sysfs_mod_ ## _id = { \
        .id = #_id, \
        .list = LIST_HEAD_INIT(_sysfs_mod_ ## _id.list), \
        .create = (_create_sysfs), \
        .remove = (_remove_sysfs), \
    }; \
    \
    static int _init_ ## _id(module_t *m) { \
        return fs_register_sysfs(&_sysfs_mod_ ## _id); \
    } \
    \
    static int _deinit_ ## _id(module_t *m) { \
        return fs_unregister_sysfs(&_sysfs_mod_ ## _id); \
    } \
    \
    MODULE(sysfs_mod_ ## _id, _init_ ## _id, _deinit_ ## _id)
