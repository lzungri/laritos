#pragma once

#include <stdbool.h>
#include <board/types.h>
#include <cpu/cpu-local.h>
#include <component/component.h>
#include <component/cpu.h>
#include <driver/core.h>
#include <time/core.h>
#include <dstruct/list.h>
#include <mm/slab.h>
#include <time/tick.h>
#include <sync/spinlock.h>
#include <sync/atomic.h>
#include <arch/core.h>
#include <generated/autoconf.h>
#include <irq/types.h>
#include <fs/vfs/types.h>

struct pcb;
typedef struct {
    /**
     * Slab for pcb_t allocation
     */
    slab_t *pcb_slab;

    /**
     * List of processes in the system
     */
    list_head_t pcbs;

    /**
     * Spinlock used to synchronize the _laritos.proc.pcbs list
     *
     * Lock ordering (if both locks are needed):
     *      pcbs_lock
     *          pcbs_data_lock
     */
    spinlock_t pcbs_lock;

    /**
     * Spinlock used to protect any pcb_t data structure that may be changed by
     * two or more processors at the same time.
     *
     * Ideally, for performance reasons, there should be one lock per pcb_t, but this greatly
     * simplifies the code and reduces synchronization complexity.
     *
     * Lock ordering (if both locks are needed):
     *      pcbs_lock
     *          pcbs_data_lock
     */
    spinlock_t pcbs_data_lock;

    /**
     * Pointer to the init process pcb_t
     */
    struct pcb *init;
} laritos_process_t;

typedef struct {
    /**
     * Process currently running on each cpu.
     *
     * Ideally, this should be part of each cpu_t, but we need access to this data structure
     * even before the cpu_t instances are created. May need to rethink this architecture
     * to improve this.
     */
    DEF_CPU_LOCAL(struct pcb *, running);

    /**
     * List of READY processes per cpu
     */
    DEF_CPU_LOCAL(list_head_t, ready_pcbs);

    /**
     * Indicates whether or not the OS should schedule the next 'ready' process
     * right before returning from the current non-user mode and when all the work required is done.
     *
     * Calling schedule() while handling an irq (e.g. without having acknowledged the int
     * controller yet), may prevent future irqs to be dispatched, will block the current irq
     * processing, among other unintentional fatal consequences.
     */
    bool need_sched;
} laritos_sched_t;

typedef struct {
    atomic32_t ctx_switches;
} laritos_stats_t;

typedef struct {
    list_head_t fstypes;
    list_head_t mounts;
    list_head_t sysfs_mods;
    fs_dentry_t *root;
    fs_dentry_t *sysfs_root;
    fs_dentry_t *proc_root;
    fs_dentry_t *mem_root;
    fs_dentry_t *slab_root;
    fs_dentry_t *stats_root;
    fs_dentry_t *sched_root;
    fs_dentry_t *comp_root;
    fs_dentry_t *comp_info_root;
} laritos_fs_t;

/**
 * laritOS Global context
 */
typedef struct {
    /**
     *
     * uint32_t instead of bool to keep the structure 4-byte aligned
     *
     * Indicates whether or not the OS has enabled the process execution mode.
     *    true: Every execution flow runs in a process context
     *    false: Every execution flow runs in the context of the kernel
     */
    uint32_t process_mode;

    /**
     * Board information
     */
    board_info_t bi;

    /**
     * List of components grouped by type (for performance reasons)
     */
    list_head_t comps[COMP_TYPE_LEN];

    /**
     * List of loaded modules
     */
    list_head_t modules;

    /**
     * List of supported drivers
     */
    list_head_t drivers;

    /**
     * List of supported loader formats (e.g. elf32, a.out, etc)
     */
    list_head_t loaders;

    /**
     * CPU shortcuts, will be initialized by cpu_init()
     */
    DEF_CPU_LOCAL(cpu_t *, cpu);

    bool components_loaded;

    laritos_process_t proc;
    laritos_sched_t sched;
    laritos_stats_t stats;
    laritos_fs_t fs;

    int32_t rndseed;

    /**
     * Time information
     */
    struct {
        timezone_t tz;
        bool dst;
        atomic64_t osticks;
        time_t boottime;
    } timeinfo;

    arch_data_t arch_data;
} laritos_t;

// Global structure used as an entry point for accessing the main pieces of information
// of the OS
extern laritos_t _laritos;

/**
 * Kernel entry point.
 *
 * Called by the bootloader
 */
extern void kernel_entry(void);
