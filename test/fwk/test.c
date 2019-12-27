#include <log.h>

#include <string.h>
#include <mm/heap.h>
#include <test/test.h>
#include <sync/spinlock.h>

#define MAX_FILEPATH_LEN 255

int test_main(void *testdescs) {
    test_descriptor_t **tests = testdescs;

    info("Tests:");

    test_ctx_t ctx = { 0 };
    test_descriptor_t **tdptr;

    uint32_t heap_avail = heap_get_available();

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
        switch(td->func()) {
        case TEST_ERROR:
            error("   [ERROR] %s", td->name);
            ctx.error++;
            break;
        case TEST_FAIL:
            error("   [FAIL] %s", td->name);
            ctx.failed++;
            break;
        case TEST_PASS:
            info("   [PASS] %s", td->name);
            ctx.passed++;
            break;
        }

        irqctx_t ctx;
        spinlock_acquire(&_laritos.proclock, &ctx);
        // Release each zombie child that may have been spawn during the test
        process_unregister_zombie_children_locked(process_get_current());
        spinlock_release(&_laritos.proclock, &ctx);
    }

    info("--------------------------------------------------");
    info("Test summary:");
    info("   Failed: %3u", ctx.failed);
    info("   Error:  %3u", ctx.error);
    info("   Passed: %3u", ctx.passed);
    info("   Total:  %3u", ctx.failed + ctx.error + ctx.passed);
    info("--------------------------------------------------");

    if (heap_avail > heap_get_available()) {
        warn("Tests may be leaking %lu bytes of heap", heap_avail - heap_get_available());
        heap_dump_info();
    }
    return 0;
}
