#include <log.h>
#include <string.h>
#include <core.h>
#include <cpu.h>
#include <mm/heap.h>
#include <component/component.h>
#include <component/inputdev.h>
#include <component/timer.h>
#include <loader/loader.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <sched/context.h>
#include <time/time.h>
#include <board-types.h>
#include <board.h>
#include <utils/debug.h>
#include <generated/utsrelease.h>
#ifdef CONFIG_TEST_ENABLED
#include <test/test.h>
#endif


laritos_t _laritos;

static void shell(void) {
    while (true) {
        component_t *c;
        for_each_component_type(c, COMP_TYPE_INPUTDEV) {
            stream_t *s = ((inputdev_t *) c)->transport;
            char buf[32];
            int bread = 0;
            if ((bread = s->ops.read(s, buf, sizeof(buf) - 1, true)) > 0) {
                buf[bread] = '\0';
                info("[%s]: %s", c->id, buf);


                switch (buf[0]) {
                case 's':
                    // system call
                    asm("mov r0, %[c]" : : [c] "r" (buf[0]));
                    asm("mov r1, #2");
                    asm("mov r2, #3");
                    asm("mov r3, #4");
                    asm("mov r4, #5");
                    asm("mov r5, #6");
                    asm("mov r6, #7");
                    dump_cur_state();
                    asm("svc 1");
                    break;
                case 'r':
                    // reset
                    asm("b 0");
                    break;
                case 'd':
                    // data abort exc
                    asm("movw r7, #0xffff");
                    asm("movt r7, #0xffff");
                    asm("str r6, [r7]");
                    break;
                case 'p':
                    // prefetch exc
                    asm("movw r7, #0xffff");
                    asm("movt r7, #0xffff");
                    asm("mov pc, r7");
                    break;
                case 'u':
                    // undef exc
                    asm(".word 0xffffffff");
                    break;
                case 'c':;
                    calendar_t c = { 0 };
                    time_rtc_get_localtime_calendar(&c);
                    log_always("calendar: %02d/%02d/%ld %02d:%02d:%02d",
                            c.mon + 1, c.mday, c.year + 1900, c.hour, c.min, c.sec);
                    break;
                case 't':;
                    // rtc timer status
                    time_t t = { 0 };
                    time_rtc_gettime(&t);
                    log_always("rtc_gettime(): %lu", (uint32_t) t.secs);

                    component_t *c1;
                    for_each_component_type(c1, COMP_TYPE_RTC) {
                        timer_comp_t *t = (timer_comp_t *) c1;
                        int64_t v;
                        t->ops.get_remaining(t, &v);
                        log_always("rtc remaining: %ld", (int32_t) v);
                    }

                    log_always("_laritos.timeinfo.ticks: %lu", (uint32_t) _laritos.timeinfo.ticks);
                    break;
                case 'm':;
                    char *p = malloc(10);
                    char *p2 = malloc(20);
                    char *p3 = malloc(30);
                    free(p2);
                    free(p);
                    free(p3);
                    heap_dump_info();
                    break;
                case 'h':;
                    // Heap buffer overflow
                    char *ptr = malloc(10);
                    ptr[10] = 0xCA;
                    ptr[11] = 0xCA;
                    ptr[12] = 0xCA;
                    ptr[13] = 0xCA;
                    free(ptr);
                    break;
                }
            }
        }
    }
}

static int initialize_global_context(void) {
    if (component_init_global_context() < 0) {
        goto error_comp;
    }

    if (pcb_init_global_context() < 0) {
        goto error_pcb;
    }
    return 0;

    pcb_deinit_global_context();
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

    if (board_parse_and_initialize(&_laritos.bi) < 0) {
        fatal("Couldn't initialize board");
    }

#ifdef CONFIG_LOG_LEVEL_DEBUG
    board_dump_board_info(&_laritos.bi);
#endif

    if (driver_process_board_components(&_laritos.bi) < 0) {
        fatal("Error processing board components");
    }

#ifdef CONFIG_LOG_LEVEL_DEBUG
    dump_registered_comps();
#endif

    if (!component_are_mandatory_comps_present()) {
        fatal("Not all mandatory board components were found");
    }

    info("Setting default timezone as PDT");
    if (time_set_timezone(TZ_PST, true) < 0) {
        error("Couldn't set default timezone");
    }

    cpu_t *c = cpu();
    if (c->ops.set_irqs_enable(c, true) < 0) {
        fatal("Failed to enable irqs for cpu %u", c->id);
    }

#ifdef CONFIG_TEST_ENABLED
    log_always("***** Running in test mode *****");
    heap_dump_info();
    if (test_run(__tests_start) < 0) {
        fatal("Error executing test cases");
    }
    heap_dump_info();
    while (1) {
        arch_wfi();
    }
#endif

    pcb_t *bigbang_pcb = loader_load_executable_from_memory(0);
    if (bigbang_pcb == NULL) {
        error("Failed to load app #0");
    }

    // Load the same program, just to have two processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #1");
    }

    // Load the same program, just to have two processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #2");
    }

    // From now on, everything will be executed in the context of a process
    _laritos.process_mode = true;

    // Execute the first process
    sched_move_to_running(bigbang_pcb);
    context_restore(bigbang_pcb);
    // Execution will never reach this point

    shell();
}
