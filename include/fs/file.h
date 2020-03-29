#pragma once

#include <stdint.h>
#include <string.h>
#include <printf.h>
#include <process/core.h>
#include <utils/math.h>
#include <fs/vfs/core.h>

static inline bool file_is_absolute(char *path) {
    return path != NULL && path[0] == '/';
}

static inline char *file_get_basename(char *path) {
    char *b = strrchr(path, '/');
    return b == NULL ? path : b + 1;
}

static inline int file_get_abs_dirname(char *path, char *buf, size_t len) {
    if (path == NULL) {
        return -1;
    }

    if (file_is_absolute(path)) {
        strncpy(buf, path, len);
    } else {
        char cwd[256];
        vfs_dentry_get_fullpath(process_get_current()->cwd, cwd, sizeof(cwd));
        snprintf(buf, len, "%s/%s", cwd, path);
    }

    char *basename = strrchr(buf, '/') + 1;
    *basename = '\0';

    return 0;
}
static inline bool file_exist(char *path) {
    return vfs_dentry_lookup(path) != NULL;
}

static inline bool file_is_dir(char *path) {
    return vfs_dentry_is_dir(vfs_dentry_lookup(path));
}
