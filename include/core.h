#pragma once

#include <stdbool.h>
#include <board-types.h>
#include <component/component.h>
#include <driver/driver.h>
#include <time/time.h>
#include <dstruct/list.h>
#include <mm/slab.h>
#include <time/tick.h>
#include <sync/spinlock.h>
#include <sync/atomic.h>

struct pcb;
typedef struct {
    /**
     * Slab for pcb_t allocation
     */
    slab_t *pcb_slab;

    /**
     * List of processes in the system
     */
    struct list_head pcbs;

    /**
     * Pointer to the init process pcb_t
     */
    struct pcb *init;
} laritos_process_t;

typedef struct {
    struct pcb *running[CONFIG_CPU_MAX_CPUS];

    /**
     * List of READY processes in the system
     */
    struct list_head ready_pcbs;

    /**
     * List of BLOCKED processes in the system
     */
    struct list_head blocked_pcbs;

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
    struct list_head comps[COMP_TYPE_LEN];

    laritos_process_t proc;
    laritos_sched_t sched;

    /**
     * Spinlock used to synchronize all process-related lists and data structures
     */
    spinlock_t proclock;

    /**
     * Time information
     */
    struct {
        timezone_t tz;
        bool dst;
        atomic64_t ticks;
    } timeinfo;
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
