#include <log.h>

#include <process/core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <mm/sysfs.h>
#include <sched/sysfs.h>
#include <generated/autoconf.h>

int fs_mount_essential_filesystems(void) {
    info("Mounting root filesystem");
    fs_mount_t *mnt = vfs_mount_fs("pseudofs", "/", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting root filesystem");
        goto error_root;
    }
    _laritos.fs.root = mnt->root;

    info("Mounting sysfs filesystem");
    mnt = vfs_mount_fs("pseudofs", "/sys", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting sysfs");
        goto error_sysfs;
    }
    _laritos.fs.sysfs_root = mnt->root;

    _laritos.fs.proc_root = vfs_dir_create(_laritos.fs.sysfs_root, "proc",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.proc_root == NULL) {
        error("Error creating proc sysfs directory");
        goto error_proc;
    }

    _laritos.fs.stats_root = vfs_dir_create(_laritos.fs.sysfs_root, "stats",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.stats_root == NULL) {
        error("Error creating stats sysfs directory");
        goto error_stats;
    }

    if (sched_create_sysfs() < 0) {
        error("Error creating sched sysfs");
        goto error_sched;
    }

    if (mem_create_sysfs() < 0) {
        error("Error creating mem sysfs");
        goto error_mem;
    }
    return 0;

//error_xxx:
//    mem_remove_sysfs();
error_mem:
    sched_remove_sysfs();
error_sched:
    vfs_dir_remove(_laritos.fs.sysfs_root, "stats");
error_stats:
    vfs_dir_remove(_laritos.fs.sysfs_root, "proc");
error_proc:
    vfs_unmount_fs("/sys");
error_sysfs:
    vfs_unmount_fs("/");
error_root:
    return -1;
}
