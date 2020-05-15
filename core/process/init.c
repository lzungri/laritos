/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <mm/heap.h>
#include <process/core.h>
#include <process/sysfs.h>
#include <cpu/core.h>
#include <assert.h>
#include <sync/spinlock.h>
#include <component/component.h>
#include <component/ticker.h>
#include <loader/loader.h>
#include <board/types.h>
#include <board/core.h>
#include <random.h>
#include <utils/debug.h>
#include <symbol.h>
#include <utils/conf.h>
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

static int launch_on_boot_processes(void) {
    info("Launching system processes");

    fs_file_t *f = vfs_file_open("/sys/conf/init/launch_on_boot.conf", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open 'launch_on_boot.conf' file");
        return -1;
    }

    char launcher[CONFIG_FS_MAX_FILENAME_LEN];
    char *tokens[] = { launcher };
    uint32_t tokens_size[] = { sizeof(launcher) };

    uint32_t offset = 0;
    int fret = 0;
    int ret;
    while ((ret = conf_readline(f, tokens, tokens_size, ARRAYSIZE(tokens), &offset)) != 0) {
        if (ret < 0) {
            continue;
        }
        if (launch_process(launcher) == NULL) {
            error("Couldn't launch %s", launcher);
            fret = -1;
        }
    }

    vfs_file_close(f);

    return fret;
}

static int complete_init_process_setup(pcb_t *init) {
    init->parent = init;
    init->cwd = _laritos.fs.root;

    irqctx_t datactx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &datactx);
    // Init holds a reference to itself to prevent unwanted process deallocations
    // (e.g. when a children process uses sleep() and then it releases its reference
    // to the init process, if the reference reaches zero, init will be deallocated)
    ref_inc(&init->refcnt);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &datactx);


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

    info("Launching idle process");
    pcb_t *idle_launcher(void);
    assert(idle_launcher() != NULL, "Couldn't launch idle process");

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

    if (vfs_mount_from_config() < 0) {
        warn("Error mounting file systems from config file, some FSs may not be mounted");
    }

    assert(launch_on_boot_processes() >= 0, "Failed to create system processes");

    assert(ticker_start_all() >= 0, "Failed to start OS tickers");

    init_loop();

    return 0;
}
