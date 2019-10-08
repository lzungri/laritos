#pragma once

#include <log.h>
#include <stdint.h>
#include <string.h>
#include <utils/utils.h>

typedef struct {
    uint16_t failed;
    uint16_t error;
    uint16_t passed;
} test_ctx_t;

typedef enum {
    TEST_ERROR = -2,
    TEST_FAIL,
    TEST_PASS,
} testres_t;

typedef testres_t (*testfunc_t)(void);

typedef struct {
    char *name;
    char *fpath;
    testfunc_t func;
} test_descriptor_t;


// Null-terminated array of pointers to test descriptors
extern test_descriptor_t *__tests_start[];

/**
 * Runs all the test cases registered via DEF_TEST macro
 *
 * @param tests: Null-terminated array of pointers to test descriptors
 * @return 0 on success, <0 on error
 */
int test_run(test_descriptor_t *tests[]);


/**
 * Defines a test case function and registers into the test cases array
 *
 * @param _test: Test name
 */
#define T(_test) \
    static testres_t _test(void); \
    static test_descriptor_t _test_desc_ ## _test = { \
        .name = #_test, \
        .fpath = __FILE__, \
        .func = (_test), \
    }; \
    test_descriptor_t * _testdesc_ ## _test ## _ptr __attribute__ ((section (".test"))) = &_test_desc_ ## _test; \
    static testres_t _test(void)

#define TEND \
        return TEST_PASS;\
    }

#define tassert(_expr) do { \
        __typeof__ (_expr) __exprres = (_expr); \
        if (!__exprres) { \
            error("[ " #_expr " ] failed in " __FILE__ ":" TOSTRING(__LINE__)); \
            return TEST_FAIL; \
        } \
    } while(0)
