#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <process/types.h>
#include <process/core.h>
#include <mm/heap.h>
#include <mm/spprot.h>
#include <sync/spinlock.h>
#include <property/core.h>
#include <generated/autoconf.h>

/**
 * Time in seconds between system health checks
 */
#define CHECK_INTERVAL_PROP "health.check_interval"
/**
 * % of available heap at which to trigger a warning
 */
#define AVAIL_HEAP_THRESH_PROP "health.thresh.availheap"
/**
 * % of available process stack at which to trigger a warning
 */
#define AVAIL_PROC_STACK_THRESH_PROP "health.thresh.availstack"


static void check_heap(void) {
    verbose("Checking heap healthy status");
    uint32_t avail = heap_get_available() * 100 / CONFIG_MEM_HEAP_SIZE;
    if (avail < (uint32_t) property_get_or_def_int32(AVAIL_HEAP_THRESH_PROP, 30)) {
        warn("Running low on heap memory, %lu%% available", avail);
    }
}

static void check_processes(void) {
    verbose("Checking processes healthy statuses");

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);

    pcb_t *proc;
    for_each_process_locked(proc) {
        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
        uint32_t stack_avail = process_get_avail_stack_locked(proc) * 100 / proc->mm.stack_size;
        bool stack_corrupted = spprot_is_stack_corrupted(proc);
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

        if (stack_avail < (uint32_t) property_get_or_def_int32(AVAIL_PROC_STACK_THRESH_PROP, 30)) {
            warn("Running low on stack for pid=%u, %lu%% available", proc->pid, stack_avail);
        }
        if (stack_corrupted) {
            warn("Stack for pid=%u is corrupted", proc->pid);
        }
    }

    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);
}

static int health_main(void *data) {
    property_create(CHECK_INTERVAL_PROP, "30",
            PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL);
    property_create(AVAIL_HEAP_THRESH_PROP, "30",
            PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL);
    property_create(AVAIL_PROC_STACK_THRESH_PROP, "30",
            PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL);

    while (1) {
        check_heap();
        check_processes();

        sleep(property_get_or_def_int32(CHECK_INTERVAL_PROP, 30));
    }
    return 0;
}

static pcb_t *launcher(proc_mod_t *pmod) {
    return process_spawn_kernel_process(pmod->id, health_main, NULL, 8196, 1);
}

PROCESS_LAUNCHER_MODULE(health, launcher)
