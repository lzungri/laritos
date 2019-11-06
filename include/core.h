#pragma once

#include <stdbool.h>
#include <board-types.h>
#include <component/component.h>
#include <driver/driver.h>
#include <time/time.h>
#include <dstruct/list.h>
#include <mm/slab.h>

typedef struct {
    /**
     * Slab for pcb_t allocation
     */
    slab_t *pcb_slab;

    /**
     * List of processes in the system
     */
    struct list_head pcbs;
} laritos_process_t;

struct pcb;
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
     * List of ZOMBIE processes in the system
     */
    struct list_head zombie_pcbs;
} laritos_sched_t;

/**
 * laritOS Global context
 */
typedef struct {
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
     * Time information
     */
    struct {
        timezone_t tz;
        bool dst;
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
