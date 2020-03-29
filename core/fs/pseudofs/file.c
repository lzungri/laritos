#define DEBUG
#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <utils/math.h>
#include <fs/pseudofs.h>

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

fs_dentry_t *pseudofs_create_custom_ro_file_with_dataptr(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t), void *data) {
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
    d->inode->file_data0 = data;
    return d;
}

fs_dentry_t *pseudofs_create_custom_ro_file(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t)) {
    return pseudofs_create_custom_ro_file_with_dataptr(parent, fname, read, NULL);
}

fs_dentry_t *pseudofs_create_custom_wo_file_with_dataptr(fs_dentry_t *parent, char *fname,
        int (*write)(fs_file_t *, void *, size_t, uint32_t), void *data) {
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
    d->inode->file_data0 = data;
    return d;
}

fs_dentry_t *pseudofs_create_custom_wo_file(fs_dentry_t *parent, char *fname,
        int (*write)(fs_file_t *, void *, size_t, uint32_t)) {
    return pseudofs_create_custom_wo_file_with_dataptr(parent, fname, write, NULL);
}

fs_dentry_t *pseudofs_create_custom_rw_file_with_dataptr(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t),
        int (*write)(fs_file_t *, void *, size_t, uint32_t), void *data) {
    fs_file_ops_t fops = {
        .open = pseudofs_def_open,
        .close = pseudofs_def_close,
        .read = read,
        .write = write,
    };
    fs_dentry_t *d = pseudofs_create_file(parent, fname, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &fops);
    if (d == NULL) {
        error("Failed to create rw '%s' sysfs file", fname);
        return NULL;
    }
    d->inode->file_data0 = data;
    return d;
}

fs_dentry_t *pseudofs_create_custom_rw_file(fs_dentry_t *parent, char *fname,
        int (*read)(fs_file_t *, void *, size_t, uint32_t),
        int (*write)(fs_file_t *, void *, size_t, uint32_t)) {
    return pseudofs_create_custom_wo_file_with_dataptr(parent, fname, write, NULL);
}

int pseudofs_raw_write_to_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset, bool check_nulls) {
    if (check_nulls && from == NULL) {
        return 0;
    }
    if (offset >= fromlen) {
        return 0;
    }
    size_t len = min(fromlen - offset, tolen);
    memcpy(to, (char *) from + offset, len);
    return len;
}

int pseudofs_write_to_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset) {
    return pseudofs_raw_write_to_buf(to, tolen, from, fromlen, offset, true);
}

int pseudofs_raw_read_from_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset, bool check_nulls) {
    if (check_nulls && to == NULL) {
        return 0;
    }
    if (offset >= tolen) {
        return 0;
    }
    size_t len = min(tolen - offset, fromlen);
    memcpy((char *) to + offset, from, len);
    return len;
}

int pseudofs_read_from_buf(void *to, size_t tolen, void *from, size_t fromlen, uint32_t offset) {
    return pseudofs_raw_read_from_buf(to, tolen, from, fromlen, offset, true);
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



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS_FILE
#include __FILE__
#endif
