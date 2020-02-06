#define DEBUG
#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

fs_dentry_t *vfs_create_dir(fs_dentry_t *parent, char *dirname, fs_access_mode_t mode) {
    verbose("Creating dir %s/%s", parent->name, dirname);
    if (parent->inode->ops.mkdir == NULL) {
        error("mkdir not supported");
        return NULL;
    }

    if (vfs_dentry_lookup_from(parent, dirname) != NULL) {
        error("Directory %s already exists", dirname);
        return NULL;
    }

    fs_dentry_t *d = vfs_dentry_alloc(dirname, NULL, parent);
    if (d == NULL) {
        error("Couldn't allocate dentry");
        return NULL;
    }

    if (parent->inode->ops.mkdir(parent->inode, d, mode) < 0) {
        error("Could not create dir %s", dirname);
        return NULL;
    }
    return d;
}

int vfs_remove_dir(fs_dentry_t *parent, char *dirname) {
    verbose("Removing dir %s/%s", parent->name, dirname);
    if (parent->inode->ops.rmdir == NULL) {
        error("rmdir not supported");
        return -1;
    }

    fs_dentry_t *d = vfs_dentry_lookup_from(parent, dirname);
    if (d == NULL) {
        error("Directory %s doesn't exist", dirname);
        return -1;
    }

    if (parent->inode->ops.rmdir(parent->inode, d) < 0) {
        error("Couldn't remove directory %s", dirname);
        return -1;
    }

    vfs_dentry_free_tree(d);
    return 0;
}

int vfs_remove_file(fs_dentry_t *parent, char *dirname) {
    return -1;
}

fs_inode_t *vfs_inode_def_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

int vfs_inode_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return -1;
}

int vfs_inode_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return -1;
}

fs_inode_t *vfs_inode_def_alloc(fs_superblock_t *sb) {
    fs_inode_t *inode = calloc(1, sizeof(fs_inode_t));
    if (inode == NULL) {
        error("No memory available for fs_inode_t structure");
        return NULL;
    }
    inode->sb = sb;
    inode->ops.lookup = vfs_inode_def_lookup;
    inode->ops.mkdir = vfs_inode_def_mkdir;
    inode->ops.rmdir = vfs_inode_def_rmdir;
    return inode;
}
