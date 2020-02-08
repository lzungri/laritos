#define DEBUG
#include <log.h>

#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

static int mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode);

static fs_inode_t *lookup(fs_inode_t *parent, char *name) {
    // TODO Check parent is a dir


    return NULL;
}

static int mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

static int rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

static int mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

static int rmregfile(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

static int def_open(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int def_close(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    int i = 0;
    fs_dentry_t *d;
    list_for_each_entry(d, &f->dentry->children, siblings) {
        if (i >= offset) {
            if (i - offset >= listlen) {
                break;
            }

            fs_listdir_t *dir = &dirlist[i - offset];
            strncpy(dir->name, d->name, sizeof(dir->name));
            dir->isdir = d->inode->mode & FS_ACCESS_MODE_DIR;
        }

        i++;
    }
    return i < offset ? 0 : i - offset;
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
    inode->ops.mkregfile = mkregfile;
    inode->ops.rmregfile = rmregfile;

    inode->fops.open = def_open;
    inode->fops.close = def_close;
    inode->fops.listdir = listdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    free(inode);
}

fs_dentry_t *pseudofs_create_file(fs_dentry_t *parent, char *fname,
                fs_access_mode_t mode, fs_file_ops_t *fops) {
    fs_dentry_t *f = vfs_file_create(parent, fname, mode);
    if (f == NULL) {
        error("Couldn't create '%s' file", fname);
        return NULL;
    }

    if (fops == NULL) {
        f->inode->fops.open = NULL;
        f->inode->fops.close = NULL;
        f->inode->fops.read = NULL;
        f->inode->fops.write = NULL;
    } else {
        f->inode->fops.open = fops->open;
        f->inode->fops.close = fops->close;
        f->inode->fops.read = fops->read;
        f->inode->fops.write = fops->write;
    }
    f->inode->fops.listdir = NULL;

    return f;
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

static int mount(fs_type_t *fstype, fs_mount_t *m) {
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
