#pragma once

#include <stdint.h>
#include <dstruct/list.h>

typedef struct {

} fs_inode_ops_t;

typedef struct {

    fs_inode_ops_t ops;
} fs_inode_t;


struct fs_superblock;
typedef struct {
    fs_inode_t *(*alloc_inode)(struct fs_superblock *sb);
} fs_superblock_ops_t;

typedef struct fs_superblock {

    fs_superblock_ops_t ops;
} fs_superblock_t;


typedef enum {
    FS_MOUNT_READ = 1,
    FS_MOUNT_WRITE = 2,
} fs_mount_flags_t;

typedef struct {
    char *mount_point;

    fs_superblock_t *sb;
} fs_mount_t;


typedef struct fs_type{
    char *id;
    struct list_head list;

    fs_mount_t *(*mount)(struct fs_type *type, char *mount_point, uint16_t flags, void *params);
} fs_type_t;
