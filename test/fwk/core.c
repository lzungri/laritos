#include <log.h>

#include <string.h>
#include <mm/heap.h>
#include <time/core.h>
#include <test/test.h>
#include <sync/spinlock.h>
#include <process/core.h>

#define MAX_FILEPATH_LEN 255

typedef struct {
    uint16_t failed;
    uint16_t error;
    uint16_t passed;
    uint16_t skipped;
    uint16_t potential_leaks;
} test_ctx_t;

/**
 * Runs all the test cases registered via DEF_TEST macro
 *
 * @param testdescs: Null-terminated array of pointers to test descriptors
 * @return 0 on success, <0 on error
 */
static int test_main(void *testdescs) {
    test_descriptor_t **tests = testdescs;

    info("Tests:");

    test_ctx_t ctx = { 0 };
    test_descriptor_t **tdptr;


    time_t suite_start;
    time_t test_start;
    time_t end;
    time_t duration;
    uint16_t hours;
    uint16_t mins;
    uint16_t secs;

    time_get_ns_rtc_time(&suite_start);

    char *fpath = "";
    for (tdptr = tests; *tdptr != NULL; tdptr++) {
        test_descriptor_t *td = *tdptr;

        if (td->func == NULL) {
            continue;
        }

        if (strncmp(td->fpath, fpath, MAX_FILEPATH_LEN) != 0) {
            fpath = td->fpath;
            info("--------------------------------------------------");
            info("%s:", fpath);
        }

        uint32_t heap_avail = heap_get_available();

        time_get_ns_rtc_time(&test_start);

        testres_t tret = td->func();

        time_get_ns_rtc_time(&end);
        time_sub(&end, &test_start, &duration);
        time_to_hms(&duration, &hours, &mins, &secs);

        if (heap_avail > heap_get_available()) {
            warn("Test may be leaking %lu bytes of heap", heap_avail - heap_get_available());
            ctx.potential_leaks++;
        }

        switch(tret) {
        case TEST_ERROR:
            error("  %-50.50s | ERROR | %02u:%02u:%02u.%03u ",
                    td->name, hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
            ctx.error++;
            break;
        case TEST_FAIL:
            error("  %-50.50s | FAIL | %02u:%02u:%02u.%03u",
                    td->name, hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
            ctx.failed++;
            break;
        case TEST_PASS:
            info("  %-50.50s | PASS | %02u:%02u:%02u.%03u",
                    td->name, hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
            ctx.passed++;
            break;
        case TEST_SKIP:
            info("  %-50.50s | SKIP | %02u:%02u:%02u.%03u",
                    td->name, hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
            ctx.skipped++;
            break;
        }

        // Release each zombie child that may have been spawn during the test
        process_unregister_zombie_children(process_get_current());
    }

    time_get_ns_rtc_time(&end);
    time_sub(&end, &suite_start, &duration);
    time_to_hms(&duration, &hours, &mins, &secs);

    info("--------------------------------------------------");
    info("Test summary:");
    info("  Failed:   %3u", ctx.failed);
    info("  Error:    %3u", ctx.error);
    info("  Passed:   %3u", ctx.passed);
    info("  Skipped:  %3u", ctx.skipped);
    info("  Total:    %3u", ctx.failed + ctx.error + ctx.passed + ctx.skipped);
    info("  Duration: %02u:%02u:%02u.%03u", hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
    info("  Leaks:    %3u", ctx.potential_leaks);
    info("--------------------------------------------------");
    return 0;
}

pcb_t *test_launcher(void) {
    return process_spawn_kernel_process("test", test_main, __tests_start,
            CONFIG_PROCESS_TEST_STACK_SIZE, CONFIG_SCHED_PRIORITY_MAX_USER - 2);
}
