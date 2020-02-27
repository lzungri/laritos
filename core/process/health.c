#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <process/types.h>
#include <process/core.h>
#include <mm/heap.h>
#include <mm/spprot.h>
#include <sync/spinlock.h>
#include <generated/autoconf.h>

static void check_heap(void) {
    verbose("Checking heap healthy status");
    uint32_t avail = heap_get_available() * 100 / CONFIG_MEM_HEAP_SIZE;
    if (avail < CONFIG_PROCESS_HEALTH_AVAIL_HEAP_WARNING_THRESHOLD) {
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

        if (stack_avail < CONFIG_PROCESS_HEALTH_AVAIL_PROC_STACK_WARNING_THRESHOLD) {
            warn("Running low on stack for pid=%u, %lu%% available", proc->pid, stack_avail);
        }
        if (stack_corrupted) {
            warn("Stack for pid=%u is corrupted", proc->pid);
        }
    }

    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);
}

static int health_main(void *data) {
    while (1) {
        check_heap();
        check_processes();

        sleep(CONFIG_PROCESS_HEALTH_CHECK_INTERVAL);
    }
    return 0;
}

static pcb_t *launcher(proc_mod_t *pmod) {
    return process_spawn_kernel_process(pmod->id, health_main, NULL, 8196, 1);
}

PROCESS_LAUNCHER_MODULE(health, launcher)
