#include <log.h>

#include <mm/heap.h>
#include <process/core.h>
#include <cpu/core.h>
#include <utils/assert.h>
#include <loader/loader.h>
#include <sync/spinlock.h>
#include <component/component.h>
#include <component/ticker.h>
#include <loader/loader.h>
#include <board/types.h>
#include <board/core.h>
#include <utils/random.h>
#include <utils/debug.h>
#include <sched/core.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <generated/autoconf.h>
#include <generated/utsrelease.h>

#ifdef CONFIG_TEST_ENABLED
#include <test/test.h>
#endif

static int mount_sysfs(void) {
    info("Mounting root filesystem");
    fs_mount_t *mnt = vfs_mount_fs("pseudofs", "/", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting root filesystem");
        goto error_root;
    }
    _laritos.fs.root = mnt->root;

    info("Mounting sysfs filesystem");
    mnt = vfs_mount_fs("pseudofs", "/sys", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    if (mnt == NULL) {
        error("Error mounting sysfs");
        goto error_sysfs;
    }
    _laritos.fs.sysfs_root = mnt->root;

    _laritos.fs.proc_root = vfs_dir_create(_laritos.fs.sysfs_root, "proc",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.proc_root == NULL) {
        error("Error creating proc sysfs directory");
        goto error_proc;
    }

    return 0;

error_proc:
    vfs_unmount_fs("/sys");
error_sysfs:
    vfs_unmount_fs("/");
error_root:
    return -1;
}

static int spawn_system_processes(void) {
    info("Spawning idle process");
    int idle_main(void *data);
    if(process_spawn_kernel_process("idle", idle_main, NULL,
            CONFIG_PROCESS_IDLE_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST) == NULL) {
        error("Could not create idle process");
        return -1;
    }

#ifdef CONFIG_TEST_ENABLED
    log_always("***** Running in test mode *****");
    // TODO Remove this process, this is just for debugging
    info("Spawning shell process");
    int shell_main(void *data);
    if (process_spawn_kernel_process("shell", shell_main, NULL,
            8196, CONFIG_SCHED_PRIORITY_MAX_USER - 10) == NULL) {
        error("Could not create shell process");
        return -1;
    };

    info("Spawning test process");
    if (process_spawn_kernel_process("test", test_main, __tests_start,
            CONFIG_PROCESS_TEST_STACK_SIZE, CONFIG_SCHED_PRIORITY_MAX_USER - 2) == NULL) {
        error("Could not create TEST process");
        return -1;
    };
#else
    // TODO: This code will disappear once we implement a shell and file system
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #0");
    }
#endif
    return 0;
}

static int start_os_tickers(void) {
    // Start OS tickers
    component_t *comp;
    for_each_component_type(comp, COMP_TYPE_TICKER) {
        ticker_comp_t *ticker = (ticker_comp_t *) comp;
        info("Starting ticker '%s'", comp->id);
        if (ticker->ops.resume(ticker) < 0) {
            error("Could not start ticker %s", comp->id);
            return -1;
        }
    }
    return 0;
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

    assert(mount_sysfs() >= 0, "Couldn't mount sysfs file system");

    assert(spawn_system_processes() >= 0, "Failed to create system processes");

    assert(start_os_tickers() >= 0, "Failed to start OS tickers");

    init_loop();
    return 0;
}
