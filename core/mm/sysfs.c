#include <log.h>

#include <printf.h>
#include <core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>
#include <mm/heap.h>

static int avail_mem_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", heap_get_available());
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int mem_create_sysfs(sysfs_mod_t *sysfs) {
    _laritos.fs.mem_root = vfs_dir_create(_laritos.fs.sysfs_root, "mem",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.mem_root == NULL) {
        error("Error creating mem sysfs directory");
        return -1;
    }

    if (pseudofs_create_custom_ro_file(_laritos.fs.mem_root, "heap_avail", avail_mem_read) == NULL) {
        error("Failed to create 'heap_avail' sysfs file");
        return -1;
    }

    return 0;
}

static int mem_remove_sysfs(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.sysfs_root, "mem");
}


SYSFS_MODULE(mem, mem_create_sysfs, mem_remove_sysfs)
