#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>

int vfs_init_global_context(void);

fs_type_t *vfs_get_fstype(char *fstype);
int vfs_register_fs_type(fs_type_t *fst);
int vfs_unregister_fs_type(fs_type_t *fst);
bool vfs_is_fs_type_supported(char *fstype);

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, fs_mount_flags_t flags, void *params);
int vfs_unmount_fs(char *mount_point);

void vfs_dentry_init(fs_dentry_t *d, char *name, fs_inode_t *inode, fs_dentry_t *parent);
fs_dentry_t *vfs_dentry_alloc(char *name, fs_inode_t *inode, fs_dentry_t *parent);
void vfs_dentry_free(fs_dentry_t *d);
void vfs_dentry_add_child(fs_dentry_t *parent, fs_dentry_t *child);
void vfs_dentry_remove_as_child(fs_dentry_t *child);
fs_dentry_t *vfs_dentry_lookup_from(fs_dentry_t *parent, char *relpath);
fs_dentry_t *vfs_dentry_lookup(char *path);
fs_dentry_t *vfs_dentry_lookup_parent(char *path);
void vfs_dentry_free_tree(fs_dentry_t *root);
bool vfs_dentry_is_dir(fs_dentry_t *d);
int vfs_dentry_get_fullpath(fs_dentry_t *d, char *buf, size_t buflen);

fs_dentry_t *vfs_dir_create(fs_dentry_t *parent, char *dirname, fs_access_mode_t mode);
int vfs_dir_remove(fs_dentry_t *parent, char *dirname);
int vfs_dir_listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen);

fs_dentry_t *vfs_file_create(fs_dentry_t *parent, char *fname, fs_access_mode_t mode);
int vfs_file_remove(fs_dentry_t *parent, char *fname);
fs_file_t *vfs_file_open(char *path, fs_access_mode_t mode);
fs_file_t *vfs_file_dentry_open(fs_dentry_t *d, fs_access_mode_t mode);
int vfs_file_close(fs_file_t *f);
int vfs_file_close_all_for_cur_process(void);
int vfs_file_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset);
int vfs_file_read_cur_offset(fs_file_t *f, void *buf, size_t blen);
int vfs_file_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset);
int vfs_file_write_cur_offset(fs_file_t *f, void *buf, size_t blen);

fs_inode_t *vfs_inode_def_alloc(fs_superblock_t *sb);
void vfs_inode_def_free(fs_inode_t *inode);


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
    MODULE(fstype_ ## _id, _init_ ## _id, _deinit_ ## _id)
