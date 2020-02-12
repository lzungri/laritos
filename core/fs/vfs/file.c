#define DEBUG
#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/slab.h>
#include <process/core.h>

static fs_file_t *vfs_file_alloc(fs_dentry_t *dentry) {
    slab_t *slab = process_get_current()->fs.fds_slab;
    fs_file_t *f = slab_alloc(slab);
    if (f == NULL) {
        error("Max number of file descriptors reached");
        return NULL;
    }
    f->dentry = dentry;
    verbose("new fd=%lu for file='%s'", slab_get_slab_position(slab, f), dentry->name);
    return f;
}

static void vfs_file_free(fs_file_t *f) {
    slab_t *slab = process_get_current()->fs.fds_slab;
    verbose("free fd=%lu for file='%s'", slab_get_slab_position(slab, f), f->dentry->name);
    slab_free(slab, f);
}

fs_file_t *vfs_file_open(char *path, fs_access_mode_t mode) {
    verbose("Opening '%s' using mode=0x%x", path, mode);
    fs_dentry_t *d = vfs_dentry_lookup(path);
    if (d == NULL) {
        error("%s doesn't exist", path);
        return NULL;
    }

    if ((d->inode->mode & mode) != mode) {
        error("Not enough permissions to open '%s'", path);
        return NULL;
    }

    if ((d->inode->mode & FS_ACCESS_MODE_WRITE) && !(d->inode->sb->mount->flags & FS_MOUNT_WRITE)) {
        error("Permission denied, mounted filesystem is read-only");
        return NULL;
    }

    if (d->inode->fops.open == NULL) {
        error("open('%s') operation not permitted", path);
        return NULL;
    }

    fs_file_t *f = vfs_file_alloc(d);
    if (f == NULL) {
        error("Failed to allocated fs_file_t struct");
        return NULL;
    }
    f->mode = mode;

    if (d->inode->fops.open(d->inode, f) < 0) {
        error("Couldn't not open file '%s'", path);
        goto error_open;
    }
    f->opened = true;

    return f;

error_open:
    vfs_file_free(f);
    return NULL;
}

int vfs_file_close(fs_file_t *f) {
    verbose("Closing '%s'", f->dentry->name);

    if (f->dentry->inode->fops.close != NULL &&
            f->dentry->inode->fops.close(f->dentry->inode, f) < 0) {
        error("Error closing '%s'", f->dentry->name);
        return -1;
    }
    f->opened = false;

    vfs_file_free(f);
    return 0;
}

int vfs_file_close_all_for_cur_process(void) {
    slab_t *slab = process_get_current()->fs.fds_slab;
    int i;
    for (i = 0; i < slab_get_total_elems(slab); i++) {
        if (slab_is_taken(slab, i)) {
            fs_file_t *f = slab_get_ptr_from_position(slab, i);
            if (f->opened) {
                vfs_file_close(f);
            }
        }
    }
    return 0;
}

int vfs_file_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    if (!f->opened) {
        error("Cannot read a closed file");
        return -1;
    }

    if (!(f->mode & FS_ACCESS_MODE_READ)) {
        error("File was not opened for reading");
        return -1;
    }

    if (f->dentry->inode->fops.read == NULL) {
        error("read('%s') operation not permitted", f->dentry->name);
        return -1;
    }

    int ret = f->dentry->inode->fops.read(f, buf, blen, offset);
    verbose("Reading %d bytes from '%s', ret=%d", blen, f->dentry->name, ret);
    return ret;
}

int vfs_file_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    if (!f->opened) {
        error("Cannot write a closed file");
        return -1;
    }

    if (!(f->mode & FS_ACCESS_MODE_WRITE)) {
        error("File was not opened for writing");
        return -1;
    }

    if (f->dentry->inode->fops.write == NULL) {
        error("write('%s') operation not permitted", f->dentry->name);
        return -1;
    }

    int ret = f->dentry->inode->fops.write(f, buf, blen, offset);
    verbose("Writing %d bytes to '%s', ret=%d", blen, f->dentry->name, ret);
    return ret;
}
