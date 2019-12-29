#include <log.h>
#include <string.h>
#include <core.h>
#include <cpu.h>
#include <mm/heap.h>
#include <component/component.h>
#include <process/core.h>
#include <sched/core.h>

laritos_t _laritos;

static int initialize_global_context(void) {
    if (component_init_global_context() < 0) {
        goto error_comp;
    }

    if (process_init_global_context() < 0) {
        goto error_pcb;
    }

    return 0;

//error_xxx:
//    process_deinit_global_context();
error_pcb:
    component_deinit_global_context();
error_comp:
    return -1;
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
                        CONFIG_PROCESS_INIT_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST - 1);
    assert(_laritos.proc.init != NULL, "Could not create init process");

    sched_execute_first_system_proc(_laritos.proc.init);

    // Execution will never reach this point
}
