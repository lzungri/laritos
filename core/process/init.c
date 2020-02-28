#include <log.h>

#include <mm/heap.h>
#include <process/core.h>
#include <process/sysfs.h>
#include <cpu/core.h>
#include <utils/assert.h>
#include <sync/spinlock.h>
#include <component/component.h>
#include <component/ticker.h>
#include <loader/loader.h>
#include <board/types.h>
#include <board/core.h>
#include <utils/random.h>
#include <utils/debug.h>
#include <sched/core.h>
#include <fs/core.h>
#include <module/core.h>
#include <generated/autoconf.h>
#include <generated/utsrelease.h>


static int complete_init_process_setup(pcb_t *init) {
    init->parent = init;
    init->cwd = _laritos.fs.root;

    // Create sysfs nodes for the init process
    return process_sysfs_create(init);
}

static void init_loop(void) {
    pcb_t *init = process_get_current();

    // Loop forever
    while (1) {
        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

        // Block and wait for events (e.g. new zombie process)
        sched_move_to_blocked_locked(init, NULL);

        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);


        // Switch to another process
        schedule();
        // Someone woke me up, process event/s

        // Release zombie children
        process_unregister_zombie_children(init);
    }
}

int init_main(void *data) {
    // We are now running in process mode, i.e. everything will be executed in the context of a process
    _laritos.process_mode = true;

    log_always("-- laritOS " UTS_RELEASE " --");
    info("Initializing kernel");
    info("Heap of %u bytes initialized at 0x%p", CONFIG_MEM_HEAP_SIZE, __heap_start);

    assert(module_load_static_modules() >= 0, "Failed to load static modules");

    assert(fs_mount_essential_filesystems() >= 0, "Couldn't mount essential filesystems");

    assert(complete_init_process_setup(process_get_current()) >= 0, "Couldn't complete setup for the init process");

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

    assert(process_spawn_system_procs() >= 0, "Failed to create system processes");

    assert(ticker_start_all() >= 0, "Failed to start OS tickers");

    init_loop();

    return 0;
}
