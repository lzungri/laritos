#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

static int mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);

static fs_inode_t *lookup(fs_inode_t *parent, char *name) {
    // TODO Check parent is a dir


    return NULL;
}

static int rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    free(dentry->inode);
    dentry->inode = NULL;
    return 0;
}

static fs_inode_t *alloc_inode(fs_superblock_t *sb) {
    fs_inode_t *inode = calloc(1, sizeof(fs_inode_t));
    if (inode == NULL) {
        error("No memory available for pseudofs_inode_t structure");
        return NULL;
    }
    inode->sb = sb;
    inode->ops.lookup = lookup;
    inode->ops.mkdir = mkdir;
    inode->ops.rmdir = rmdir;
    return (fs_inode_t *) inode;
}

static int alloc_inode_and_fill_dentry(fs_dentry_t *d, fs_superblock_t *sb, fs_access_mode_t mode) {
    fs_inode_t *inode = alloc_inode(sb);
    if (inode == NULL) {
        error("No memory for fs_inode_t");
        return -1;
    }
    inode->mode = mode;
    d->inode = inode;
    return 0;
}

static int mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return alloc_inode_and_fill_dentry(dentry, parent->sb, mode | FS_ACCESS_MODE_DIR);
}

static void free_inode(fs_inode_t *inode) {
    free(inode);
}

fs_dentry_t *pseudofs_create_file(fs_dentry_t *parent, char *fname,
                fs_access_mode_t mode, fs_file_ops_t *fops) {
    if (parent == NULL || parent->inode == NULL || !vfs_dentry_is_dir(parent)) {
        error("Parent is null or not a dir");
        return NULL;
    }
    verbose("Creating file %s/%s", parent->name, dirname);

    if (vfs_dentry_lookup_from(parent, fname) != NULL) {
        error("File %s already exists", fname);
        return NULL;
    }

    fs_dentry_t *d = vfs_dentry_alloc(fname, NULL, parent);
    if (d == NULL) {
        error("Couldn't allocate dentry");
        return NULL;
    }

    if (alloc_inode_and_fill_dentry(d, parent->inode->sb, mode) < 0) {
        error("Couldn't allocate inode for '%s' file", fname);
        goto error_inode;
    }

    if (fops != NULL) {
        d->inode->fops.open = fops->open;
        d->inode->fops.read = fops->read;
        d->inode->fops.write = fops->write;
    }

    return d;

error_inode:
    vfs_dentry_free(d);
    return NULL;
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

int mount(fs_type_t *fstype, fs_mount_t *m) {
    m->sb = calloc(1, sizeof(fs_superblock_t));
    if (m->sb == NULL) {
        error("No memory available for fs_superblock_t structure");
        goto error_sb;
    }
    m->sb->fstype = fstype;
    m->sb->ops.alloc_inode = alloc_inode;
    m->sb->ops.free_inode = free_inode;

    m->ops.unmount = unmount;

    return 0;

error_sb:
    return -1;
}

FILESYSTEM_MODULE(pseudofs, mount);



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS
#include __FILE__
#endif
