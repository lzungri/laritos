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
#include <mm/heap.h>

static int ctxswitches_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", atomic32_get(&_laritos.stats.ctx_switches));
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int osticks_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", (uint32_t) atomic64_get(&_laritos.timeinfo.osticks));
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int create_root_sysfs(fs_sysfs_mod_t *sysfs) {
    _laritos.fs.sched_root = vfs_dir_create(_laritos.fs.stats_root, "sched",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.sched_root == NULL) {
        error("Error creating sched sysfs directory");
        return -1;
    }

    if (pseudofs_create_custom_ro_file(_laritos.fs.sched_root, "ctxswitches", ctxswitches_read) == NULL) {
        error("Failed to create 'ctxswitches' sysfs file");
        return -1;
    }

    if (pseudofs_create_custom_ro_file(_laritos.fs.sched_root, "osticks", osticks_read) == NULL) {
        error("Failed to create 'osticks' sysfs file");
        return -1;
    }

    return 0;
}

static int remove_root_sysfs(fs_sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.stats_root, "sched");
}


SYSFS_MODULE(sched, create_root_sysfs, remove_root_sysfs)
