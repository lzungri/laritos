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

#include <printf.h>
#include <core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <dstruct/list.h>
#include <mm/heap.h>
#include <component/blockdev.h>
#include <generated/autoconf.h>

static int mount_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[512];
    uint32_t totalb = 0;
    fs_mount_t *mnt = NULL;
    list_for_each_entry(mnt, &_laritos.fs.mounts, list) {
        if (sizeof(data) - totalb < 32) {
            break;
        }

        char devoffset[12] = { 0 };
        if (mnt->sb->dev != NULL && mnt->sb->devoffset > 0) {
            snprintf(devoffset, sizeof(devoffset), "0x%08x", mnt->sb->devoffset);
        }

        char mntpoint[CONFIG_FS_MAX_FILENAME_LEN];
        vfs_dentry_get_fullpath(mnt->root, mntpoint, sizeof(mntpoint));
        int strlen = snprintf(data + totalb, sizeof(data) - totalb, "%-10.10s %-10.10s %-10.10s %-10.10s %s%s\n",
                mntpoint,
                mnt->sb->dev != NULL ? mnt->sb->dev->parent.id : "<nodev>", devoffset,
                mnt->sb->fstype->id, mnt->flags & FS_MOUNT_READ ? "r" : "",
                mnt->flags & FS_MOUNT_WRITE ? "w" : "");
        if (strlen < 0) {
            return -1;
        }
        totalb += strlen;
    }

    return pseudofs_write_to_buf(buf, blen, data, totalb, offset);
}

static int create_sysfs(fs_sysfs_mod_t *sysfs) {
    if (pseudofs_create_custom_ro_file(_laritos.fs.root, "mount", mount_read) == NULL) {
        error("Failed to create 'mount' sysfs file");
        return -1;
    }
    return 0;
}

static int remove_sysfs(fs_sysfs_mod_t *sysfs) {
    return vfs_file_remove(_laritos.fs.root, "mount");
}


SYSFS_MODULE(mount, create_sysfs, remove_sysfs)
