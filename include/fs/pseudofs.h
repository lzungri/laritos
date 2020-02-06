#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>

fs_dentry_t *pseudofs_create_file(fs_dentry_t *parent, char *fname, fs_access_mode_t mode, fs_file_ops_t *fops);
