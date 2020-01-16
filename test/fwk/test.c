#include <log.h>

#include <string.h>
#include <mm/heap.h>
#include <time/time.h>
#include <test/test.h>
#include <sync/spinlock.h>

#define MAX_FILEPATH_LEN 255

int test_main(void *testdescs) {
    test_descriptor_t **tests = testdescs;

    info("Tests:");

    test_ctx_t ctx = { 0 };
    test_descriptor_t **tdptr;

    uint32_t heap_avail = heap_get_available();

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

        time_get_ns_rtc_time(&test_start);

        testres_t tret = td->func();

        time_get_ns_rtc_time(&end);
        time_sub(&end, &test_start, &duration);
        time_to_hms(&duration, &hours, &mins, &secs);

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
        }

        irqctx_t ctx;
        spinlock_acquire(&_laritos.proclock, &ctx);
        // Release each zombie child that may have been spawn during the test
        process_unregister_zombie_children_locked(process_get_current());
        spinlock_release(&_laritos.proclock, &ctx);
    }

    time_get_ns_rtc_time(&end);
    time_sub(&end, &suite_start, &duration);
    time_to_hms(&duration, &hours, &mins, &secs);

    info("--------------------------------------------------");
    info("Test summary:");
    info("  Failed:   %3u", ctx.failed);
    info("  Error:    %3u", ctx.error);
    info("  Passed:   %3u", ctx.passed);
    info("  Total:    %3u", ctx.failed + ctx.error + ctx.passed);
    info("  Duration: %02u:%02u:%02u.%03u", hours, mins, secs, (uint16_t) NS_TO_MS(duration.ns));
    info("--------------------------------------------------");

    if (heap_avail > heap_get_available()) {
        warn("Tests may be leaking %lu bytes of heap", heap_avail - heap_get_available());
        heap_dump_info();
    }
    return 0;
}
