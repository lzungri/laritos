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

#include <core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <generated/autoconf.h>
#include <generated/utsrelease.h>


static int create_sysfs(fs_sysfs_mod_t *sysfs) {
    if (pseudofs_create_bin_file(_laritos.fs.root, "kver", FS_ACCESS_MODE_READ, "laritOS-" UTS_RELEASE, sizeof("laritOS-" UTS_RELEASE)) == NULL) {
        error("Failed to create 'kver' sysfs file");
        return -1;
    }
    return 0;
}

static int remove_sysfs(fs_sysfs_mod_t *sysfs) {
    return vfs_file_remove(_laritos.fs.root, "kver");
}


SYSFS_MODULE(core, create_sysfs, remove_sysfs)
