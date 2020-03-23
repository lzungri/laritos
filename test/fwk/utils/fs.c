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

