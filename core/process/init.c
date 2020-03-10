#include <log.h>

#include <stdbool.h>
#include <string.h>
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
#include <utils/symbol.h>
#include <sched/core.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <generated/autoconf.h>
#include <generated/utsrelease.h>


static pcb_t *launch_process(char *launcher) {
    if (launcher[0] == '/') {
        info("Launching process from binary '%s'", launcher);
        return loader_load_executable_from_file(launcher);
    }

    info("Launching process from symbol '%s'", launcher);
    pcb_t *(*func)(void) = symbol_get(launcher);
    if (func == NULL) {
        error("No symbol '%s' found, check your launch_on_boot.conf file", launcher);
        return NULL;
    }
    return func();
}

static int spawn_system_procs(void) {
    info("Launching system processes");

    fs_file_t *f = vfs_file_open("/sys/conf/init/launch_on_boot.conf", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open kernel symbols file");
        return -1;
    }

    int ret = 0;
    uint8_t linepos = 0;
    uint32_t offset = 0;
    char launcher[CONFIG_FS_MAX_FILENAME_LEN] = { 0 };
    while (true) {
        char buf[256];
        int nbytes = vfs_file_read(f, buf, sizeof(buf), offset);
        if (nbytes <= 0) {
            break;
        }

        int i;
        for (i = 0; i < nbytes; i++) {
            if (buf[i] == '\n') {
                // Make sure we have collected some data at least, otherwise ignore the line
                if (strlen(launcher) == 0) {
                    linepos = 0;
                    continue;
                }

                if (launch_process(launcher) == NULL) {
                    error("Couldn't launch %s", launcher);
                    ret = -1;
                }

                linepos = 0;
                memset(launcher, 0, sizeof(launcher));
            } else {
                if (linepos < sizeof(launcher) - 1) {
                    launcher[linepos++] = buf[i];
                }
            }
        }

        offset += nbytes;
    }

    // Launch last line process (in case there was no '\n')
    if (strlen(launcher) > 0) {
        if (launch_process(launcher) == NULL) {
            error("Couldn't launch %s", launcher);
            ret = -1;
        }
    }

    vfs_file_close(f);

    return ret;
}

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

    assert(vfs_mount_essential_filesystems() >= 0, "Couldn't mount essential filesystems");

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

    assert(spawn_system_procs() >= 0, "Failed to create system processes");

    assert(ticker_start_all() >= 0, "Failed to start OS tickers");

    init_loop();

    return 0;
}
