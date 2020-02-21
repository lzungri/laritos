#pragma once

#include <log.h>

#include <stdint.h>
#include <string.h>

#include <dstruct/list.h>
#include <core.h>
#include <refcount.h>
#include <mm/slab.h>
#include <process/status.h>
#include <time/tick.h>
#include <sync/atomic.h>
#include <sync/condition.h>
#include <syscall/syscall-no.h>
#include <fs/vfs/types.h>
#include <generated/autoconf.h>

typedef struct {
    void *imgaddr;
    secsize_t imgsize;

    void *text_start;
    secsize_t text_size;
    void *data_start;
    secsize_t data_size;
    void *got_start;
    secsize_t got_size;
    void *bss_start;
    secsize_t bss_size;
    void *heap_start;
    secsize_t heap_size;
    void *stack_bottom;
    secsize_t stack_size;

    spctx_t *sp_ctx;
} pcb_mm_t;

typedef struct {
    process_status_t status;
    uint8_t priority;

    /**
     * Monotonic start time in nano seconds
     */
    time_t start_time;

    /**
     * Node used to link a process to the _laritos.proc.pcbs list
     *
     * Protected by _laritos.proc.pcbs_lock
     */
    list_head_t pcb_node;

    /**
     * Node used to link a process to a particular scheduling list. So far,
     * it could be a per-cpu READY list or a blocked list (if the blocking
     * event has any).
     */
    list_head_t sched_node;
} pcb_sched_t;

typedef struct {
    slab_t *fds_slab;
} pcb_fs_t;

typedef struct {
    abstick_t last_status_change;
    tick_t ticks_spent[PROC_STATUS_LEN];
    atomic32_t syscalls[SYSCALL_LEN];
} pcb_stats_t;

typedef struct pcb {
    uint16_t pid;
    char name[CONFIG_PROCESS_MAX_NAME_LEN];
    bool kernel;

    char cmd[CONFIG_PROCESS_MAX_CMD_LEN];
    fs_dentry_t *cwd;
    pcb_mm_t mm;
    pcb_sched_t sched;
    pcb_fs_t fs;
    pcb_stats_t stats;

    int exit_status;

    struct pcb *parent;
    condition_t parent_waiting_cond;
    list_head_t children;
    list_head_t siblings;

    refcount_t refcnt;
} pcb_t;


/**
 * Kernel process main function type
 *
 * @param data: Pointer to any data you want to send to the kernel function
 */
typedef int (*kproc_main_t)(void *data);
