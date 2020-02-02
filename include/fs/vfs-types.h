#pragma once

#include <stdint.h>
#include <dstruct/list.h>
#include <generated/autoconf.h>


typedef enum {
    FS_ACCESS_MODE_READ = 1,
    FS_ACCESS_MODE_WRITE = 2,
    FS_ACCESS_MODE_EXEC = 4,
} fs_access_mode_t;

struct fs_mount;
typedef struct fs_type {
    char *id;
    struct list_head list;

    struct fs_mount *(*mount)(struct fs_type *fstype, char *mount_point, uint16_t flags, void *params);
} fs_type_t;

struct inode;
typedef struct {
    struct inode *(*lookup)(struct inode *parent, char *name);
    int (*mkdir)(struct inode *parent, char *name, fs_access_mode_t mode);
    int (*rmdir)(struct inode *parent, char *name);
} fs_inode_ops_t;

typedef struct inode {

    fs_inode_ops_t ops;
} fs_inode_t;


struct dentry;
typedef struct {

} fs_dentry_ops_t;

typedef struct dentry {
    fs_inode_t *inode;
    char name[CONFIG_FS_MAX_FILENAME_LEN];
    struct dentry *parent;
    struct list_head children;
    struct list_head siblings;

    fs_dentry_ops_t ops;
} fs_dentry_t;


struct fs_superblock;
typedef struct {
    fs_inode_t *(*alloc_inode)(struct fs_superblock *sb);
    void (*free_inode)(fs_inode_t *inode);
} fs_superblock_ops_t;

typedef struct fs_superblock {
    fs_type_t *fstype;

    fs_superblock_ops_t ops;
} fs_superblock_t;


typedef enum {
    FS_MOUNT_READ = 1,
    FS_MOUNT_WRITE = 2,
} fs_mount_flags_t;

struct fs_mount;
typedef struct {
    int (*unmount)(struct fs_mount *fsm);
} fs_mount_ops_t;

typedef struct fs_mount {
    fs_dentry_t root;
    fs_mount_flags_t flags;
    struct list_head list;
    fs_superblock_t *sb;

    fs_mount_ops_t ops;
} fs_mount_t;
