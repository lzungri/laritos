#include <log.h>

#include <printf.h>
#include <process/types.h>
#include <process/core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>
#include <time/core.h>
#include <sync/spinlock.h>
#include <generated/autoconf.h>

static int name_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    int ret = pseudofs_write_to_buf(buf, blen, pcb->name, sizeof(pcb->name), offset);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return ret;
}

static int cwd_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    char cwd[256];
    vfs_dentry_get_fullpath(pcb->cwd, cwd, sizeof(cwd));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, cwd, sizeof(cwd), offset);
}

static int running_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_RUNNING]);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int ready_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_READY]);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int blocked_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_BLOCKED]);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int start_time_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    time_t t;
    time_get_monotonic_time(&t);
    time_sub(&t, &pcb->sched.start_time, &t);
    uint16_t hours, mins, secs;
    time_to_hms(&t, &hours, &mins, &secs);

    char start[16];
    int strlen = snprintf(start, sizeof(start), "%02u:%02u:%02u", hours, mins, secs);

    return pseudofs_write_to_buf(buf, blen, start, strlen + 1, offset);
}

int process_sysfs_create(pcb_t *pcb) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%u", pcb->pid);
    fs_dentry_t *dir = vfs_dir_create(_laritos.fs.proc_root, buf,
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating sysfs directory for pid=%u", pcb->pid);
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "name", name_read, pcb) == NULL) {
        error("Failed to create 'name' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_bool_file(dir, "kernel", FS_ACCESS_MODE_READ, &pcb->kernel) == NULL) {
        error("Failed to create 'kernel' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_str_file(dir, "cmd", FS_ACCESS_MODE_READ,
            pcb->cmd, sizeof(pcb->cmd)) == NULL) {
        error("Failed to create 'cmd' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "cwd", cwd_read, pcb) == NULL) {
        error("Failed to create 'cwd' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "running", running_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "ready", ready_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "blocked", blocked_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "start_time", start_time_read, pcb) == NULL) {
        error("Failed to create 'start_time' sysfs file for pid=%u", pcb->pid);
    }

    return 0;
}

int process_sysfs_remove(pcb_t *pcb) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%u", pcb->pid);
    return vfs_dir_remove(_laritos.fs.proc_root, buf);
}


static int process_create_sysfs_root(sysfs_mod_t *sysfs) {
    _laritos.fs.proc_root = vfs_dir_create(_laritos.fs.sysfs_root, "proc",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.proc_root == NULL) {
        error("Error creating proc sysfs directory");
        return -1;
    }
    return 0;
}

static int process_remove_sysfs_root(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.sysfs_root, "proc");
}

SYSFS_MODULE(proc, process_create_sysfs_root, process_remove_sysfs_root)
