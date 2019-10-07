#include <log.h>

#include <test/test.h>

int test_run(test_descriptor_t *tests[]) {
    info("--------------------------------------------------");
    info("Tests:");

    test_ctx_t ctx = { 0 };
    test_descriptor_t **tdptr;
    for (tdptr = tests; *tdptr != NULL; tdptr++) {
        test_descriptor_t *td = *tdptr;

        if (td->func == NULL) {
            continue;
        }

        switch(td->func()) {
        case TEST_ERROR:
            error("  [ERROR] %s", td->name);
            ctx.error++;
            break;
        case TEST_FAIL:
            error("  [FAIL] %s", td->name);
            ctx.failed++;
            break;
        case TEST_PASS:
            info("  [PASS] %s", td->name);
            ctx.passed++;
            break;
        }
    }

    info("Test summary:");
    info("  Failed: %3u", ctx.failed);
    info("  Error:  %3u", ctx.error);
    info("  Passed: %3u", ctx.passed);
    info("  -----------");
    info("  Total:  %3u", ctx.failed + ctx.error + ctx.passed);
    info("--------------------------------------------------");

    return 0;
}
