#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>

int vfs_init_global_context(void);
int vfs_register_fs_type(fs_type_t *fst);
int vfs_unregister_fs_type(fs_type_t *fst);
bool vfs_is_fs_mounted(char *mount_point);
bool vfs_is_fs_type_supported(char *fstype);
void vfs_initialize_mount_struct(fs_mount_t *mount, fs_type_t *fstype, char *mount_point, uint16_t flags, void *params);
fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params);
int vfs_unmount_fs(char *mount_point);


#define FILESYSTEM_MODULE(_id, _mount) \
    static fs_type_t _fstype_ ## _id = { \
        .id = #_id, \
        .list = LIST_HEAD_INIT(_fstype_ ## _id.list), \
        .mount = (_mount), \
    }; \
    \
    static int _init_ ## _id(module_t *m) { \
        return vfs_register_fs_type(&_fstype_ ## _id); \
    } \
    \
    static int _deinit_ ## _id(module_t *m) { \
        return vfs_unregister_fs_type(&_fstype_ ## _id); \
    } \
    \
    MODULE(_id, _init_ ## _id, _deinit_ ## _id)
