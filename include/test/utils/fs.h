#pragma once

#include <stdbool.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

#define DATA_TEST_DIR "/data/test"

bool fs_file_in_listdir(char *dirpath, char *fname);

static inline fs_dentry_t fs_get_data_testdir(void) {
    return vfs_dentry_lookup(DATA_TEST_DIR);
}
