#include <log.h>

#include <printf.h>
#include <core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <mm/heap.h>

static int avail_mem_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", heap_get_available());
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

int mem_create_sysfs(void) {
    _laritos.fs.mem_root = vfs_dir_create(_laritos.fs.sysfs_root, "mem",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.mem_root == NULL) {
        error("Error creating mem sysfs directory");
        return -1;
    }

    if (pseudofs_create_custom_ro_file(_laritos.fs.mem_root, "heap_avail", avail_mem_read) == NULL) {
        error("Failed to create 'heap_avail' sysfs file");
    }

    heap_create_sysfs();
//    slab_create_sysfs();

    return 0;
}

int mem_remove_sysfs(void) {
//    slab_remove_sysfs();
    heap_remove_sysfs();
    return vfs_dir_remove(_laritos.fs.sysfs_root, "mem");
}
