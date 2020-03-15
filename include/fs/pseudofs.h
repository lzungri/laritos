#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sync/atomic.h>
#include <fs/vfs/types.h>

typedef struct {
    fs_superblock_t parent;

    atomic32_t next_inode_number;
} pseudofs_sb_t;

fs_inode_t *pseudofs_def_lookup(fs_inode_t *parent, char *name);
int pseudofs_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);
int pseudofs_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry);
int pseudofs_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);
int pseudofs_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry);
int pseudofs_def_open(fs_inode_t *inode, fs_file_t *f);
int pseudofs_def_close(fs_inode_t *inode, fs_file_t *f);

fs_dentry_t *pseudofs_create_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, fs_file_ops_t *fops);
fs_dentry_t *pseudofs_create_custom_ro_file(fs_dentry_t *parent, char *fname, int (*read)(fs_file_t *, void *, size_t, uint32_t));
fs_dentry_t *pseudofs_create_custom_ro_file_with_dataptr(fs_dentry_t *parent, char *fname, int (*read)(fs_file_t *, void *, size_t, uint32_t), void *data);
fs_dentry_t *pseudofs_create_custom_wo_file(fs_dentry_t *parent, char *fname, int (*write)(fs_file_t *, void *, size_t, uint32_t));
fs_dentry_t *pseudofs_create_custom_wo_file_with_dataptr(fs_dentry_t *parent, char *fname, int (*write)(fs_file_t *, void *, size_t, uint32_t), void *data);
fs_dentry_t *pseudofs_create_custom_rw_file_with_dataptr(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t),
        int (*write)(fs_file_t *, void *, size_t, uint32_t),
        void *data);
fs_dentry_t *pseudofs_create_custom_rw_file(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t),
        int (*write)(fs_file_t *, void *, size_t, uint32_t));


/**
 * WARNING: Not thread-safe. No synchronization mechanism will be used to protect
 * the <value> argument. If <value> is part of a critical section, using these file
 * utilities may lead to race conditions.
 * If you need to enforce mutual exclusion, use pseudofs_create_file() instead
 */
fs_dentry_t *pseudofs_create_bin_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, void *value, size_t size);

int pseudofs_write_to_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset);
int pseudofs_raw_write_to_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset, bool check_nulls);
int pseudofs_read_from_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset);
int pseudofs_raw_read_from_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset, bool check_nulls);

#define DECL_PSEUDOFS_FILE_FUNC(_type) \
    static inline fs_dentry_t *pseudofs_create_ ## _type ##_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, void *value) { \
        return pseudofs_create_bin_file(parent, fname, mode, value, sizeof(_type)); \
    }

// Just an alias to bin pseudo file
#define pseudofs_create_str_file pseudofs_create_bin_file

DECL_PSEUDOFS_FILE_FUNC(bool)
DECL_PSEUDOFS_FILE_FUNC(uint8_t)
DECL_PSEUDOFS_FILE_FUNC(uint16_t)
DECL_PSEUDOFS_FILE_FUNC(uint32_t)
DECL_PSEUDOFS_FILE_FUNC(uint64_t)
DECL_PSEUDOFS_FILE_FUNC(int8_t)
DECL_PSEUDOFS_FILE_FUNC(int16_t)
DECL_PSEUDOFS_FILE_FUNC(int32_t)
DECL_PSEUDOFS_FILE_FUNC(int64_t)
