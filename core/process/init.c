#include <log.h>

#include <mm/heap.h>
#include <process/core.h>
#include <arch/cpu.h>
#include <utils/assert.h>
#include <loader/loader.h>
#include <sync/spinlock.h>
#include <component/component.h>
#include <loader/loader.h>
#include <process/core.h>
#include <board-types.h>
#include <board.h>
#include <utils/debug.h>
#include <generated/autoconf.h>
#include <generated/utsrelease.h>

#ifdef CONFIG_TEST_ENABLED
#include <test/test.h>
#endif

static void spawn_system_processes(void) {
    info("Spawning idle process");
    int idle_main(void *data);
    pcb_t *idle = process_spawn_kernel_process("idle", idle_main, NULL,
                        CONFIG_PROCESS_IDLE_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST);
    assert(idle != NULL, "Could not create idle process");

    // TODO Remove this process, this is just for debugging
    info("Spawning shell process");
    int shell_main(void *data);
    pcb_t *shell = process_spawn_kernel_process("shell", shell_main, NULL,
                        8196, CONFIG_SCHED_PRIORITY_LOWEST - 10);
    assert(shell != NULL, "Could not create shell process");

#ifdef CONFIG_TEST_ENABLED
    log_always("***** Running in test mode *****");
    info("Spawning test process");
    pcb_t *test = process_spawn_kernel_process("test", test_main, __tests_start,
                        CONFIG_PROCESS_TEST_STACK_SIZE, CONFIG_SCHED_PRIORITY_MAX_USER - 2);
    assert(test != NULL, "Could not create TEST process");
#else
    // Launch a few processes for testing
    // TODO: This code will disappear once we implement a shell and file system

    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #0");
    }

    // Load the same program, just to have 2 processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #1");
    }

    // Load the same program, just to have 3 processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #2");
    }
#endif
}

int init_main(void *data) {
    // We are now running in process mode, i.e. everything will be executed in the context of a process
    _laritos.process_mode = true;

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

    spawn_system_processes();

    // Loop forever
    while (1) {
        arch_wfi();

        irqctx_t ctx;
        spinlock_acquire(&_laritos.proclock, &ctx);
        process_unregister_zombie_children_locked(process_get_current());
        spinlock_release(&_laritos.proclock, &ctx);
    }
    return 0;
}
