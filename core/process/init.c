#include <log.h>

#include <mm/heap.h>
#include <process/core.h>
#include <cpu/cpu.h>
#include <utils/assert.h>
#include <loader/loader.h>
#include <sync/spinlock.h>
#include <component/component.h>
#include <component/ticker.h>
#include <loader/loader.h>
#include <process/core.h>
#include <board/board-types.h>
#include <board/board.h>
#include <utils/random.h>
#include <utils/debug.h>
#include <sched/core.h>
#include <fs/vfs-core.h>
#include <module/core.h>
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
                        8196, CONFIG_SCHED_PRIORITY_MAX_USER - 10);
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
    int i;
    for (i = 0; i < 5; i++) {
        if (loader_load_executable_from_memory(0) == NULL) {
            error("Failed to load app #%d", i);
        }
    }
#endif
}

static void start_os_tickers(void) {
    // Start OS tickers
    component_t *comp;
    for_each_component_type(comp, COMP_TYPE_TICKER) {
        ticker_comp_t *ticker = (ticker_comp_t *) comp;
        info("Starting ticker '%s'", comp->id);
        assert(ticker->ops.resume(ticker) >= 0, "Could not start ticker %s", comp->id);
    }
}

static void init_loop(void) {
    pcb_t *init = process_get_current();

    // Loop forever
    while (1) {
        irqctx_t ctx;
        irq_disable_local_and_save_ctx(&ctx);
        // Block and wait for events (e.g. new zombie process)
        sched_move_to_blocked_locked(init, NULL);
        irq_local_restore_ctx(&ctx);

        // Switch to another process
        schedule();

        // Someone woke me up, process event/s

        // Release zombie children
        irq_disable_local_and_save_ctx(&ctx);
        process_unregister_zombie_children_locked(init);
        irq_local_restore_ctx(&ctx);
    }
}

int init_main(void *data) {
    // We are now running in process mode, i.e. everything will be executed in the context of a process
    _laritos.process_mode = true;

    log_always("-- laritOS " UTS_RELEASE " --");
    info("Initializing kernel");
    info("Heap of %u bytes initialized at 0x%p", CONFIG_MEM_HEAP_SIZE, __heap_start);

    assert(module_load_static_modules() >= 0, "Failed to load static modules");

    assert(board_parse_and_initialize(&_laritos.bi) >= 0, "Couldn't initialize board");

#ifdef CONFIG_LOG_LEVEL_DEBUG
    board_dump_board_info(&_laritos.bi);
#endif

    assert(driver_process_board_components(&_laritos.bi) >= 0, "Error processing board components");

#ifdef CONFIG_LOG_LEVEL_DEBUG
    debug_dump_registered_comps();
#endif

    // TODO Once we implement SMP, we should do this for each cpu
    assert(cpu_initialize() >= 0, "Failed to initialize cpu #%u", cpu_get_id());

    // Save the RTC boot time. Useful for calculating the current time with nanoseconds
    // resolution (rtc just provides second resolution)
    time_get_rtc_time(&_laritos.timeinfo.boottime);

    info("Setting default timezone as PDT");
    if (time_set_timezone(TZ_PST, false) < 0) {
        error("Couldn't set default timezone");
    }

    // Seed random generator from current time
    random_seed((uint32_t) _laritos.timeinfo.boottime.secs);

    // Initialize virtual file system
    assert(vfs_init() >= 0, "Failed to initialize VFS");

    spawn_system_processes();

    start_os_tickers();

    init_loop();
    return 0;
}
