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

#include <stdbool.h>
#include <printf.h>
#include <core.h>
#include <board/core.h>
#include <component/component.h>
#include <component/blockdev.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>


int blockdev_component_init(blockdev_t *blk, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(blockdev_t *blk, void *buf, size_t n, uint32_t offset),
        int (*write)(blockdev_t *blk, void *buf, size_t n, uint32_t offset)) {
    if (component_init((component_t *) blk, bcomp->id, bcomp, COMP_TYPE_BLOCKDEV, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }
    if (read == NULL || write == NULL) {
        error("Cannot create block device with NULL read/write interface");
        return -1;
    }
    blk->ops.read = read;
    blk->ops.write = write;

    if (board_get_int_attr(bcomp, "sectorsize", (int *) &blk->sector_size) < 0) {
        error("Invalid or no sectorsize specified in the board info");
        return -1;
    }

    if (board_get_int_attr(bcomp, "nsectors", (int *) &blk->nsectors) < 0) {
        error("Invalid or no nsectors specified in the board info");
        return -1;
    }

    blk->size_kbs = (blk->nsectors * blk->sector_size) >> 10;

    return 0;
}

static int sysfs_raw_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    blockdev_t *blk = f->data0;
    return blk->ops.read(blk, buf, blen, offset);
}

static int sysfs_raw_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    blockdev_t *blk = f->data0;
    return blk->ops.write(blk, buf, blen, offset);
}

static int sysfs_nsectors(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    blockdev_t *blk = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", blk->nsectors);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int sysfs_sectorsize(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    blockdev_t *blk = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", blk->sector_size);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int create_instance_sysfs(blockdev_t *blk) {
    fs_dentry_t *root = vfs_dentry_lookup_from(_laritos.fs.comp_type_root, "blockdev");
    fs_dentry_t *dir = vfs_dir_create(root, blk->parent.id, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating '%s' sysfs directory", blk->parent.id);
        return -1;
    }

    if (pseudofs_create_custom_rw_file_with_dataptr(dir, "raw", sysfs_raw_read, sysfs_raw_write, blk) == NULL) {
        error("Failed to create 'data' sysfs file");
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "nsectors", sysfs_nsectors, blk) == NULL) {
        error("Failed to create 'nsectors' sysfs file");
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "sectorsize", sysfs_sectorsize, blk) == NULL) {
        error("Failed to create 'sectorsize' sysfs file");
        return -1;
    }

    return 0;
}

int blockdev_component_register(blockdev_t *blk) {
    if (component_register((component_t *) blk) < 0) {
        error("Couldn't register '%s'", blk->parent.id);
        return -1;
    }
    create_instance_sysfs(blk);
    return 0;
}

SYSFS_COMPONENT_TYPE_MODULE(blockdev)
