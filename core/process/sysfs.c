#include <log.h>

#include <printf.h>
#include <process/types.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <time/core.h>

static int start_time_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    time_t t;
    time_get_monotonic_time(&t);
    time_sub(&t, &f->pcb->sched.start_time, &t);
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

    if (pseudofs_create_str_file(dir, "name", FS_ACCESS_MODE_READ,
            pcb->name, sizeof(pcb->name)) == NULL) {
        error("Failed to create 'name' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_uint16_t_file(dir, "pid", FS_ACCESS_MODE_READ, &pcb->pid) == NULL) {
        error("Failed to create 'pid' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_bool_file(dir, "kernel", FS_ACCESS_MODE_READ, &pcb->kernel) == NULL) {
        error("Failed to create 'kernel' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_str_file(dir, "cmd", FS_ACCESS_MODE_READ,
            pcb->cmd, sizeof(pcb->cmd)) == NULL) {
        error("Failed to create 'cmd' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_str_file(dir, "cwd", FS_ACCESS_MODE_READ,
            pcb->cwd, sizeof(pcb->cwd)) == NULL) {
        error("Failed to create 'cwd' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_uint32_t_file(dir, "running", FS_ACCESS_MODE_READ,
            &pcb->stats.ticks_spent[PROC_STATUS_RUNNING]) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_uint32_t_file(dir, "ready", FS_ACCESS_MODE_READ,
            &pcb->stats.ticks_spent[PROC_STATUS_READY]) == NULL) {
        error("Failed to create 'ready' sysfs file for pid=%u", pcb->pid);
    }
    if (pseudofs_create_uint32_t_file(dir, "blocked", FS_ACCESS_MODE_READ,
            &pcb->stats.ticks_spent[PROC_STATUS_BLOCKED]) == NULL) {
        error("Failed to create 'blocked' sysfs file for pid=%u", pcb->pid);
    }

    if (pseudofs_create_custom_ro_file(dir, "start_time", start_time_read) == NULL) {
        error("Failed to create 'start_time' sysfs file for pid=%u", pcb->pid);
    }

    return 0;
}

int process_sysfs_remove(pcb_t *pcb) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%u", pcb->pid);
    return vfs_dir_remove(_laritos.fs.proc_root, buf);
}
