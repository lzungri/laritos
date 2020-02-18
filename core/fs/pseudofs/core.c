#define DEBUG
#include <log.h>

#include <stdint.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>
#include <utils/math.h>
#include <fs/pseudofs.h>

fs_inode_t *pseudofs_def_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

int pseudofs_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

int pseudofs_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

int pseudofs_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

int pseudofs_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

int pseudofs_def_open(fs_inode_t *inode, fs_file_t *f) {
    f->data0 = inode->file_data0;
    f->data1 = inode->file_data1;
    return 0;
}

int pseudofs_def_close(fs_inode_t *inode, fs_file_t *f) {
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

    inode->ops.lookup = pseudofs_def_lookup;
    inode->ops.mkdir = pseudofs_def_mkdir;
    inode->ops.rmdir = pseudofs_def_rmdir;
    inode->ops.mkregfile = pseudofs_def_mkregfile;
    inode->ops.rmregfile = pseudofs_def_rmregfile;

    inode->fops.open = pseudofs_def_open;
    inode->fops.close = pseudofs_def_close;
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

fs_dentry_t *pseudofs_create_custom_ro_file(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t)) {
    fs_file_ops_t fops = {
        .open = pseudofs_def_open,
        .close = pseudofs_def_close,
        .read = read,
    };
    fs_dentry_t *d = pseudofs_create_file(parent, fname, FS_ACCESS_MODE_READ, &fops);
    if (d == NULL) {
        error("Failed to create read-only '%s' sysfs file", fname);
        return NULL;
    }
    return d;
}

fs_dentry_t *pseudofs_create_custom_wo_file(fs_dentry_t *parent, char *fname,
        int (*write)(fs_file_t *, void *, size_t, uint32_t)) {
    fs_file_ops_t fops = {
        .open = pseudofs_def_open,
        .close = pseudofs_def_close,
        .write = write,
    };
    fs_dentry_t *d = pseudofs_create_file(parent, fname, FS_ACCESS_MODE_WRITE, &fops);
    if (d == NULL) {
        error("Failed to create write-only '%s' sysfs file", fname);
        return NULL;
    }
    return d;
}

int pseudofs_write_to_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset) {
    if (from == NULL) {
        return 0;
    }
    if (offset >= fromlen) {
        return 0;
    }
    size_t len = min(fromlen - offset, tolen);
    memcpy(to, (char *) from + offset, len);
    return len;
}

int pseudofs_read_from_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset) {
    if (to == NULL) {
        return 0;
    }
    if (offset >= tolen) {
        return 0;
    }
    size_t len = min(tolen - offset, fromlen);
    memcpy((char *) to + offset, from, len);
    return len;
}

static int read_bin(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    return pseudofs_write_to_buf(buf, blen, f->data0, (size_t) f->data1, offset);
}

static int write_bin(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    return pseudofs_read_from_buf(f->data0, (size_t) f->data1, buf, blen, offset);
}

fs_dentry_t *pseudofs_create_bin_file(fs_dentry_t *parent, char *fname,
                fs_access_mode_t mode, void *value, size_t size) {
    fs_file_ops_t fops = {
        .open = pseudofs_def_open,
        .close = pseudofs_def_close,
    };
    if (mode & FS_ACCESS_MODE_READ) {
        fops.read = read_bin;
    }
    if (mode & FS_ACCESS_MODE_WRITE) {
        fops.write = write_bin;
    }

    fs_dentry_t *d = pseudofs_create_file(parent, fname, mode, &fops);
    if (d == NULL) {
        error("Couldn't create file");
        return NULL;
    }
    d->inode->file_data0 = value;
    d->inode->file_data1 = (void *) size;
    return d;
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
