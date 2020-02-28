#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <dstruct/list.h>
#include <generated/autoconf.h>


typedef enum {
    FS_ACCESS_MODE_EXEC = 1,
    FS_ACCESS_MODE_WRITE = 2,
    FS_ACCESS_MODE_READ = 4,
    FS_ACCESS_MODE_DIR = 8,
} fs_access_mode_t;

struct fs_superblock;
struct fs_mount;
struct fs_inode;
struct fs_dentry;
struct fs_file;

typedef struct {
    char name[CONFIG_FS_MAX_FILENAME_LEN];
    bool isdir;
} fs_listdir_t;

typedef struct {
    int (*open)(struct fs_inode *inode, struct fs_file *f);
    int (*close)(struct fs_inode *inode, struct fs_file *f);
    int (*read)(struct fs_file *f, void *buf, size_t blen, uint32_t offset);
    int (*write)(struct fs_file *f, void *buf, size_t blen, uint32_t offset);
    /**
     * @return: -1 on error, otherwise the number of directory entries filled up.
     *      If return value equals to <listlen>, that means there may be more entries left
     *      to be read in subsequent calls
     *
     */
    int (*listdir)(struct fs_file *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen);
} fs_file_ops_t;

struct pcb;
typedef struct fs_file {
    struct pcb *pcb;
    struct fs_dentry *dentry;
    fs_access_mode_t mode;
    bool opened;
    uint32_t offset;

    void *data0;
    void *data1;
} fs_file_t;


typedef struct fs_type {
    char *id;
    list_head_t list;

    int (*mount)(struct fs_type *fstype, struct fs_mount *mount);
} fs_type_t;


typedef struct {
    struct fs_inode *(*lookup)(struct fs_inode *parent, char *name);
    int (*mkregfile)(struct fs_inode *parent, struct fs_dentry *dentry, fs_access_mode_t mode);
    int (*rmregfile)(struct fs_inode *parent, struct fs_dentry *dentry);
    int (*mkdir)(struct fs_inode *parent, struct fs_dentry *dentry, fs_access_mode_t mode);
    int (*rmdir)(struct fs_inode *parent, struct fs_dentry *dentry);
} fs_inode_ops_t;

typedef struct fs_inode {
    fs_access_mode_t mode;
    struct fs_superblock *sb;
    void *file_data0;
    void *file_data1;

    fs_inode_ops_t ops;
    fs_file_ops_t fops;
} fs_inode_t;


typedef struct {

} fs_dentry_ops_t;

typedef struct fs_dentry {
    fs_inode_t *inode;
    char name[CONFIG_FS_MAX_FILENAME_LEN];
    struct fs_dentry *parent;
    list_head_t children;
    list_head_t siblings;

    fs_dentry_ops_t ops;
} fs_dentry_t;


struct fs_superblock;
typedef struct {
    fs_inode_t *(*alloc_inode)(struct fs_superblock *sb);
    void (*free_inode)(fs_inode_t *inode);
} fs_superblock_ops_t;

typedef struct fs_superblock {
    fs_type_t *fstype;
    struct fs_mount *mount;

    fs_superblock_ops_t ops;
} fs_superblock_t;


typedef enum {
    FS_MOUNT_WRITE = 1,
    FS_MOUNT_READ = 2,
} fs_mount_flags_t;

struct fs_mount;
typedef struct {
    int (*unmount)(struct fs_mount *fsm);
} fs_mount_ops_t;

typedef struct fs_mount {
    fs_dentry_t *root;
    fs_mount_flags_t flags;
    list_head_t list;
    fs_superblock_t *sb;

    fs_mount_ops_t ops;
} fs_mount_t;
