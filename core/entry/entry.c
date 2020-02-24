#include <log.h>
#include <string.h>
#include <core.h>
#include <cpu/core.h>
#include <mm/heap.h>
#include <component/component.h>
#include <process/core.h>
#include <module/core.h>
#include <loader/loader.h>
#include <fs/vfs/core.h>
#include <sched/core.h>
#include <sync/atomic.h>
#include <utils/utils.h>

laritos_t _laritos;

static int initialize_global_context(void) {
    if (component_init_global_context() < 0) {
        while(1);
    }

    if (process_init_global_context() < 0) {
        while(1);
    }

    if (loader_init_global_context() < 0) {
        while(1);
    }

    if (module_init_global_context() < 0) {
        while(1);
    }

    if (driver_init_global_context() < 0) {
        while(1);
    }

    if (vfs_init_global_context() < 0) {
        while(1);
    }

    atomic32_init(&_laritos.stats.ctx_switches, 0);

    return 0;
}

void kernel_entry(void)  {
    if (heap_initialize(__heap_start, CONFIG_MEM_HEAP_SIZE) < 0) {
        while(1);
    }

    if (initialize_global_context() < 0) {
        while(1);
    }

    int init_main(void *data);
    _laritos.proc.init = process_spawn_kernel_process("init", init_main, NULL,
                        CONFIG_PROCESS_INIT_STACK_SIZE, 0 /* Max priority */);
    assert(_laritos.proc.init != NULL, "Could not create init process");

    sched_execute_first_system_proc(_laritos.proc.init);

    // Execution will never reach this point
}
