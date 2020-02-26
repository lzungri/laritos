#include <log.h>

#include <printf.h>
#include <core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>
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

static int sched_create_sysfs(sysfs_mod_t *sysfs) {
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

static int sched_remove_sysfs(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.stats_root, "sched");
}


SYSFS_MODULE(sched, sched_create_sysfs, sched_remove_sysfs)
