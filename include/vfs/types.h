#pragma once

#include <stdint.h>

typedef struct {

} fs_mount_t;

typedef struct fs_type{
    char *id;

    fs_mount_t *(*mount)(struct fs_type *type, char *mount_point, uint16_t flags, void *params);

} fs_type_t;
