#include <log.h>
#include <string.h>
#include <core.h>
#include <cpu.h>
#include <mm/heap.h>
#include <component/component.h>
#include <loader/loader.h>
#include <process/core.h>
#include <sched/core.h>
#include <sched/context.h>
#include <board-types.h>
#include <board.h>
#include <utils/debug.h>
#include <generated/utsrelease.h>
#ifdef CONFIG_TEST_ENABLED
#include <test/test.h>
#endif


laritos_t _laritos;

static int initialize_global_context(void) {
    if (component_init_global_context() < 0) {
        goto error_comp;
    }

    if (process_init_global_context() < 0) {
        goto error_pcb;
    }
    return 0;

    process_deinit_global_context();
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

    log_always("-- laritOS " UTS_RELEASE " --");
    info("Initializing kernel");
    info("Heap of %u bytes initialized at 0x%p", CONFIG_MEM_HEAP_SIZE, __heap_start);

    assert(board_parse_and_initialize(&_laritos.bi) >= 0, "Couldn't initialize board");

#ifdef CONFIG_LOG_LEVEL_DEBUG
    board_dump_board_info(&_laritos.bi);
#endif

    assert(driver_process_board_components(&_laritos.bi) >= 0, "Error processing board components");

#ifdef CONFIG_LOG_LEVEL_DEBUG
    debug_dump_registered_comps();
#endif

    assert(component_are_mandatory_comps_present(), "Not all mandatory board components were found");

    info("Setting default timezone as PDT");
    if (time_set_timezone(TZ_PST, true) < 0) {
        error("Couldn't set default timezone");
    }

    cpu_t *c = cpu();
    if (c->ops.set_irqs_enable(c, true) < 0) {
        fatal("Failed to enable irqs for cpu %u", c->id);
    }

    info("Starting init process");
    int init_main(void *data);
    pcb_t *init = process_spawn_kernel_process("init", init_main, NULL,
                        CONFIG_PROCESS_INIT_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST - 1);
    assert(init != NULL, "Could not create init process");

    sched_execute_first_system_proc(init);
    // Execution will never reach this point
}
