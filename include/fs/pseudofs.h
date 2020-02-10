#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>

fs_inode_t *pseudofs_def_lookup(fs_inode_t *parent, char *name);
int pseudofs_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);
int pseudofs_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry);
int pseudofs_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);
int pseudofs_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry);
int pseudofs_def_open(fs_inode_t *inode, fs_file_t *f);
int pseudofs_def_close(fs_inode_t *inode, fs_file_t *f);
fs_dentry_t *pseudofs_create_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, fs_file_ops_t *fops);
fs_dentry_t *pseudofs_create_bin_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, void *value, size_t size);
