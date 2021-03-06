/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <string.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/slab.h>
#include <process/core.h>

static fs_file_t *vfs_file_alloc(fs_dentry_t *dentry) {
    pcb_t *pcb = process_get_current();
    slab_t *slab = pcb->fs.fds_slab;
    fs_file_t *f = slab_alloc(slab);
    if (f == NULL) {
        error("Max number of file descriptors reached");
        return NULL;
    }
    memset(f, 0, sizeof(*f));
    f->dentry = dentry;
    f->pcb = pcb;
    verbose("new fd=%lu for file='%s'", slab_get_slab_position(slab, f), dentry->name);
    return f;
}

static void vfs_file_free(fs_file_t *f) {
    slab_t *slab = process_get_current()->fs.fds_slab;
    verbose_async("free fd=%lu for file='%s'", slab_get_slab_position(slab, f), f->dentry->name);
    slab_free(slab, f);
}

fs_file_t *vfs_file_dentry_open(fs_dentry_t *d, fs_access_mode_t mode) {
    if (d == NULL) {
        error("Null dentry");
        return NULL;
    }

    if ((d->inode->mode & mode) != mode) {
        error("Not enough permissions to open '%s'", d->name);
        return NULL;
    }

    if ((mode & FS_ACCESS_MODE_WRITE) && !(d->inode->sb->mount->flags & FS_MOUNT_WRITE)) {
        error("Permission denied, mounted filesystem is read-only");
        return NULL;
    }

    if (d->inode->fops.open == NULL) {
        error("open('%s') operation not permitted", d->name);
        return NULL;
    }

    fs_file_t *f = vfs_file_alloc(d);
    if (f == NULL) {
        error("Failed to allocated fs_file_t struct");
        return NULL;
    }
    f->mode = mode;

    if (d->inode->fops.open(d->inode, f) < 0) {
        error("Couldn't not open file '%s'", d->name);
        goto error_open;
    }
    f->opened = true;

    return f;

error_open:
    vfs_file_free(f);
    return NULL;
}

fs_file_t *vfs_file_open(char *path, fs_access_mode_t mode) {
    verbose("Opening '%s' using mode=0x%x", path, mode);
    fs_dentry_t *d = vfs_dentry_lookup(path);
    if (d == NULL) {
        error("%s not found", path);
        return NULL;
    }
    return vfs_file_dentry_open(d, mode);
}

int vfs_file_close(fs_file_t *f) {
    verbose("Closing '%s'", f->dentry->name);

    if (f->dentry != NULL && f->dentry->inode != NULL && f->dentry->inode->fops.close != NULL &&
            f->dentry->inode->fops.close(f->dentry->inode, f) < 0) {
        error_async("Error closing '%s'", f->dentry->name);
        return -1;
    }
    f->opened = false;

    vfs_file_free(f);
    return 0;
}

int vfs_file_close_all_for_cur_process(void) {
    pcb_t *pcb = process_get_current();
    slab_t *slab = pcb->fs.fds_slab;
    int i;
    for (i = 0; i < slab_get_total_elems(slab); i++) {
        if (slab_is_taken(slab, i)) {
            fs_file_t *f = slab_get_ptr_from_position(slab, i);
            if (f->opened) {
                warn_async("pid=%u is about to die with fd=%d open", pcb->pid, i);
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

    if (f->dentry->inode->mode & FS_ACCESS_MODE_DIR) {
        error("Cannot read a directory");
        return -1;
    }

    if (f->dentry->inode->fops.read == NULL) {
        error("read('%s') operation not permitted", f->dentry->name);
        return -1;
    }

    int ret = f->dentry->inode->fops.read(f, buf, blen, offset);
    verbose("Reading %d bytes from '%s' at offset=%lu, ret=%d", blen, f->dentry->name, offset, ret);
    return ret;
}

int vfs_file_read_cur_offset(fs_file_t *f, void *buf, size_t blen) {
    int nbytes = vfs_file_read(f, buf, blen, f->offset);
    if (nbytes > 0) {
        f->offset += nbytes;
    }
    return nbytes;
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

    if (f->dentry->inode->mode & FS_ACCESS_MODE_DIR) {
        error("Cannot write directly into a directory content");
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

int vfs_file_write_cur_offset(fs_file_t *f, void *buf, size_t blen) {
    int nbytes = vfs_file_write(f, buf, blen, f->offset);
    if (nbytes > 0) {
        f->offset += nbytes;
    }
    return nbytes;
}
