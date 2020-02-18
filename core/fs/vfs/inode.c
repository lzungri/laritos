//#define DEBUG
#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

static fs_dentry_t *_do_create(fs_dentry_t *parent, char *name, fs_access_mode_t mode,
        int (*createfunc)(fs_inode_t *, fs_dentry_t *, fs_access_mode_t)) {
    if (parent == NULL || parent->inode == NULL ||
            createfunc == NULL || !vfs_dentry_is_dir(parent)) {
        error("Operation not supported or parent is null or not a dir");
        return NULL;
    }

    if ((parent->inode->mode & FS_ACCESS_MODE_WRITE) == 0) {
        error("Permission denied");
        return NULL;
    }

    if (!(parent->inode->sb->mount->flags & FS_MOUNT_WRITE)) {
        error("Permission denied, mounted filesystem is read-only");
        return NULL;
    }

    verbose("Creating %s/%s with mode=0x%0x", parent->name, name, mode);

    if (vfs_dentry_lookup_from(parent, name) != NULL) {
        error("File %s already exists", name);
        return NULL;
    }

    fs_dentry_t *d = vfs_dentry_alloc(name, NULL, parent);
    if (d == NULL) {
        error("Couldn't allocate dentry");
        return NULL;
    }

    if (parent->inode->sb == NULL || parent->inode->sb->ops.alloc_inode == NULL ||
            (d->inode = parent->inode->sb->ops.alloc_inode(parent->inode->sb)) == NULL) {
        error("Couldn't allocate inode for '%s'", name);
        goto error_inode;
    }
    d->inode->mode = mode;

    if (createfunc(parent->inode, d, mode) < 0) {
        error("Could not create '%s'", name);
        return NULL;
    }
    return d;

error_inode:
    vfs_dentry_free(d);
    return NULL;
}

static int _do_remove(fs_dentry_t *parent, char *name,
        int (*removefunc)(fs_inode_t *, fs_dentry_t *)) {
    verbose("Removing %s/%s", parent->name, name);
    if (removefunc == NULL) {
        error("Operation not supported");
        return -1;
    }

    if ((parent->inode->mode & FS_ACCESS_MODE_WRITE) == 0) {
        error("Permission denied");
        return -1;
    }

    if (!(parent->inode->sb->mount->flags & FS_MOUNT_WRITE)) {
        error("Permission denied, mounted filesystem is read-only");
        return -1;
    }

    fs_dentry_t *d = vfs_dentry_lookup_from(parent, name);
    if (d == NULL) {
        error("'%s' doesn't exist", name);
        return -1;
    }

    if (removefunc(parent->inode, d) < 0) {
        error("Couldn't remove '%s'", name);
        return -1;
    }

    vfs_dentry_free_tree(d);
    return 0;
}

fs_dentry_t *vfs_dir_create(fs_dentry_t *parent, char *dirname, fs_access_mode_t mode) {
    if (parent == NULL || parent->inode == NULL) {
        error("mkdir not supported or parent is null");
        return NULL;
    }
    return _do_create(parent, dirname, mode | FS_ACCESS_MODE_DIR, parent->inode->ops.mkdir);
}

int vfs_dir_remove(fs_dentry_t *parent, char *dirname) {
    return _do_remove(parent, dirname, parent->inode->ops.rmdir);
}

int vfs_dir_listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    fs_access_mode_t expected_mode = FS_ACCESS_MODE_DIR | FS_ACCESS_MODE_EXEC | FS_ACCESS_MODE_READ;
    if ((f->dentry->inode->mode & expected_mode) != expected_mode ||
            f->dentry->inode->fops.listdir == NULL) {
        error("listdir() operation not permitted");
        return -1;
    }

    verbose("Listing dir of '%s'", f->dentry->name);

    return f->dentry->inode->fops.listdir(f, offset, dirlist, listlen);
}

fs_dentry_t *vfs_file_create(fs_dentry_t *parent, char *fname, fs_access_mode_t mode) {
    if (parent == NULL || parent->inode == NULL) {
        error("Create file not supported or parent is null");
        return NULL;
    }
    return _do_create(parent, fname, mode, parent->inode->ops.mkregfile);
}

int vfs_file_remove(fs_dentry_t *parent, char *fname) {
    return _do_remove(parent, fname, parent->inode->ops.rmregfile);
}

static fs_inode_t *vfs_inode_def_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

static int vfs_inode_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return -1;
}

static int vfs_inode_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return -1;
}

static int vfs_inode_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return -1;
}

static int vfs_inode_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry) {
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
    inode->ops.mkregfile = vfs_inode_def_mkregfile;
    inode->ops.rmregfile = vfs_inode_def_rmregfile;
    return inode;
}

void vfs_inode_def_free(fs_inode_t *inode) {
    free(inode);
}
