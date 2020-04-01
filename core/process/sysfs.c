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

#include <printf.h>
#include <cpu/core.h>
#include <process/types.h>
#include <process/core.h>
#include <process/status.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <sched/context.h>
#include <time/core.h>
#include <sync/spinlock.h>
#include <arch/debug.h>
#include <utils/symbol.h>
#include <generated/autoconf.h>


#define SYSFS_DEF_READ(_name, _datalen, _fmt, _expr) \
static int _name##_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) { \
    pcb_t *pcb = f->data0; \
    irqctx_t ctx; \
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx); \
    char data[_datalen]; \
    int strlen = snprintf(data, sizeof(data), _fmt, _expr); \
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx); \
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);\
}

SYSFS_DEF_READ(prio, 16, "%u", pcb->sched.priority)
SYSFS_DEF_READ(ppid, 16, "%u", pcb->parent->pid)
SYSFS_DEF_READ(running, 16, "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_RUNNING])
SYSFS_DEF_READ(ready, 16, "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_READY])
SYSFS_DEF_READ(blocked, 16, "%lu", (uint32_t) pcb->stats.ticks_spent[PROC_STATUS_BLOCKED])
SYSFS_DEF_READ(status, 16, "%s", pcb_get_status_str(pcb->sched.status))

static int name_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    int ret = pseudofs_write_to_buf(buf, blen, pcb->name, sizeof(pcb->name), offset);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return ret;
}

static int pc_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    char data[64];
    regpc_t pc = pcb->sched.status == PROC_STATUS_RUNNING ? arch_cpu_get_pc() : arch_context_get_retaddr(pcb->mm.sp_ctx);
    char symbol[32] = { 0 };
    symbol_get_name_at(pc, symbol, sizeof(symbol));

    int strlen = snprintf(data, sizeof(data), "0x%p %s", pc, symbol);

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int availstack_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    uint32_t avail = process_get_avail_stack_locked(pcb);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    char data[64];
    int strlen = snprintf(data, sizeof(data), "%lu bytes (%lu percent)", avail, (avail * 100) / pcb->mm.stack_size);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int mode_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    pcb_t *pcb = f->data0;
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    char data[64];
    char mode[64] = { 0 };
    int strlen = snprintf(data, sizeof(data), "%s",
                    pcb->sched.status == PROC_STATUS_RUNNING ?
                        arch_debug_get_psr_str(arch_cpu_get_cpsr(), mode, sizeof(mode)) :
                        arch_debug_get_psr_str_from_ctx(pcb->mm.sp_ctx, mode, sizeof(mode)));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
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

static int syscalls_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[512];
    uint32_t totalb = 0;
    int i;
    for (i = 0; i < SYSCALL_LEN; i++) {
        pcb_t *pcb = f->data0;
        int strlen = snprintf(data + totalb, sizeof(data) - totalb, "%d %ld\n", i, atomic32_get(&pcb->stats.syscalls[i]));
        if (strlen < 0) {
            return -1;
        }
        totalb += strlen;
    }
    return pseudofs_write_to_buf(buf, blen, data, totalb, offset);
}

static int maps_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    char data[512];
    uint32_t totalb = 0;
    int strlen;

#define MAP(_sect, _start, _size) \
    strlen = snprintf(data + totalb, sizeof(data) - totalb, "%-5.5s 0x%p-0x%p %lu%\n", \
            #_sect, _start, (char *) _start + _size, _size); \
    if (strlen < 0) { \
        return -1; \
    } \
    totalb += strlen;

    pcb_t *pcb = f->data0;
    MAP(img, pcb->mm.imgaddr, pcb->mm.imgsize);
    MAP(text, pcb->mm.text_start, pcb->mm.text_size);
    MAP(data, pcb->mm.data_start, pcb->mm.data_size);
    MAP(bss, pcb->mm.bss_start, pcb->mm.bss_size);
    MAP(got, pcb->mm.got_start, pcb->mm.got_size);
    MAP(heap, pcb->mm.heap_start, pcb->mm.heap_size);
    MAP(stack, pcb->mm.stack_bottom, pcb->mm.stack_size);

    return pseudofs_write_to_buf(buf, blen, data, totalb, offset);
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
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "ppid", ppid_read, pcb) == NULL) {
        error("Failed to create 'ppid' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "prio", prio_read, pcb) == NULL) {
        error("Failed to create 'maps' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_bool_file(dir, "kernel", FS_ACCESS_MODE_READ, &pcb->kernel) == NULL) {
        error("Failed to create 'kernel' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_str_file(dir, "cmd", FS_ACCESS_MODE_READ,
            pcb->cmd, sizeof(pcb->cmd)) == NULL) {
        error("Failed to create 'cmd' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "cwd", cwd_read, pcb) == NULL) {
        error("Failed to create 'cwd' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "running", running_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "ready", ready_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "blocked", blocked_read, pcb) == NULL) {
        error("Failed to create 'running' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "start_time", start_time_read, pcb) == NULL) {
        error("Failed to create 'start_time' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "syscalls", syscalls_read, pcb) == NULL) {
        error("Failed to create 'syscalls' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "maps", maps_read, pcb) == NULL) {
        error("Failed to create 'maps' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "status", status_read, pcb) == NULL) {
        error("Failed to create 'status' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "pc", pc_read, pcb) == NULL) {
        error("Failed to create 'pc' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "mode", mode_read, pcb) == NULL) {
        error("Failed to create 'mode' sysfs file for pid=%u", pcb->pid);
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "avail_stack", availstack_read, pcb) == NULL) {
        error("Failed to create 'avail_stack' sysfs file for pid=%u", pcb->pid);
        return -1;
    }

    return 0;
}

int process_sysfs_remove(pcb_t *pcb) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%u", pcb->pid);
    return vfs_dir_remove(_laritos.fs.proc_root, buf);
}


static int create_root_sysfs(fs_sysfs_mod_t *sysfs) {
    fs_mount_t *mnt = vfs_mount_fs("pseudofs", "/proc", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting proc pseudo fs");
        return -1;
    }
    _laritos.fs.proc_root = mnt->root;
    return 0;
}

static int remove_root_sysfs(fs_sysfs_mod_t *sysfs) {
    return vfs_unmount_fs("/proc");
}

SYSFS_MODULE(proc, create_root_sysfs, remove_root_sysfs)
