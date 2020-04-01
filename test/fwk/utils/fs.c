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
#include <test/utils/fs.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

bool fs_file_in_listdir(char *dirpath, char *fname) {
    int offset = 0;
    int nentries;
    fs_listdir_t dirs[8];

    fs_file_t *d = vfs_file_open(dirpath, FS_ACCESS_MODE_READ);
    if (d == NULL) {
        return false;
    }

    do {
        nentries = vfs_dir_listdir(d, offset, dirs, ARRAYSIZE(dirs));
        if (nentries <= 0) {
            goto end;
        }
        offset += nentries;
        int i;
        for (i = 0; i < nentries; i++) {
            if (strncmp(dirs[i].name, fname, sizeof(dirs[i].name)) == 0) {
                vfs_file_close(d);
                return true;
            }
        }
    } while (nentries == ARRAYSIZE(dirs));

end:
    vfs_file_close(d);
    return false;
}

