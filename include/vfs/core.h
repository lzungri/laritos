#pragma once

#include <stdint.h>
#include <vfs/types.h>

int vfs_register_fs_type(fs_type_t *fst);
fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, uint16_t flags, void *params);
