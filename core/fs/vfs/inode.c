#define DEBUG
#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

fs_dentry_t *vfs_dir_create(fs_dentry_t *parent, char *dirname, fs_access_mode_t mode) {
    if (parent == NULL || parent->inode == NULL ||
            parent->inode->ops.mkdir == NULL || !vfs_dentry_is_dir(parent)) {
        error("mkdir not supported or parent is null or not a dir");
        return NULL;
    }
    verbose("Creating dir %s/%s", parent->name, dirname);

    if (vfs_dentry_lookup_from(parent, dirname) != NULL) {
        error("Directory %s already exists", dirname);
        return NULL;
    }

    fs_dentry_t *d = vfs_dentry_alloc(dirname, NULL, parent);
    if (d == NULL) {
        error("Couldn't allocate dentry");
        return NULL;
    }

    if (parent->inode->sb == NULL || parent->inode->sb->ops.alloc_inode == NULL ||
            (d->inode = parent->inode->sb->ops.alloc_inode(parent->inode->sb)) == NULL) {
        error("Couldn't allocate inode for '%s' directory", dirname);
        goto error_inode;
    }
    d->inode->mode = mode | FS_ACCESS_MODE_DIR;

    if (parent->inode->ops.mkdir(parent->inode, d, mode) < 0) {
        error("Could not create dir %s", dirname);
        return NULL;
    }
    return d;

error_inode:
    vfs_dentry_free(d);
    return NULL;
}

int vfs_dir_remove(fs_dentry_t *parent, char *dirname) {
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

fs_dentry_t *vfs_file_create(fs_dentry_t *parent, char *fname, fs_access_mode_t mode) {
    if (parent == NULL || parent->inode == NULL || !vfs_dentry_is_dir(parent)) {
        error("Parent is null or not a dir");
        return NULL;
    }
    verbose("Creating file %s/%s", parent->name, fname);

    if (vfs_dentry_lookup_from(parent, fname) != NULL) {
        error("File %s already exists", fname);
        return NULL;
    }

    fs_dentry_t *d = vfs_dentry_alloc(fname, NULL, parent);
    if (d == NULL) {
        error("Couldn't allocate dentry");
        return NULL;
    }

    if (parent->inode->sb == NULL || parent->inode->sb->ops.alloc_inode == NULL ||
            (d->inode = parent->inode->sb->ops.alloc_inode(parent->inode->sb)) == NULL) {
        error("Couldn't allocate inode for '%s' file", fname);
        goto error_inode;
    }
    d->inode->mode = mode;

    return d;

error_inode:
    vfs_dentry_free(d);
    return NULL;
}

int vfs_file_remove(fs_dentry_t *parent, char *fname) {
    verbose("Removing file %s/%s", parent->name, fname);
    if (parent == NULL || parent->inode == NULL) {
        error("Parent is null or doesn't have an inode associated");
        return -1;
    }

    fs_dentry_t *d = vfs_dentry_lookup_from(parent, fname);
    if (d == NULL) {
        error("File %s doesn't exist", fname);
        return -1;
    }

    if (parent->inode->sb->ops.free_inode != NULL) {
        parent->inode->sb->ops.free_inode(d->inode);
    }
    vfs_dentry_free(d);

    return 0;
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

void vfs_inode_def_free(fs_inode_t *inode) {
    free(inode);
}
