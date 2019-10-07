#pragma once

#include <stdint.h>

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


#define DEF_TEST(_test) \
    static testres_t _test(void); \
    static test_descriptor_t _test_desc_ ## _test = { \
        .name = #_test, \
        .func = (_test), \
    }; \
    test_descriptor_t * _testdesc_ ## _test ## _ptr __attribute__ ((section (".test"))) = &_test_desc_ ## _test; \
    static testres_t _test(void)


#define ASSERT_EQUALS_MSG(_expr, _v, _msg, ...) \
    __typeof__ (_expr) __exprc = (_expr); \
    __typeof__ (_expr) __vc = (_v); \
    if (__exprc != __vc) { \
        error(_msg, ##__VA_ARGS__); \
        return TEST_FAIL; \
    }

#define ASSERT_EQUALS(_expr, _v) \
    ASSERT_EQUALS_MSG(_expr, _v, "Values are not equal")

#define ASSERT_TRUE_MSG(_expr, _msg, ...) \
    if (!(_expr)) { \
        error(_msg, ##__VA_ARGS__); \
        return TEST_FAIL; \
    }

#define ASSERT_TRUE(_expr) \
    ASSERT_TRUE_MSG(_expr, "Expression is not true")

#define ASSERT_NULL_MSG(_expr, _msg, ...) \
    ASSERT_EQUALS_MSG(_expr, NULL, _msg, ##__VA_ARGS__)

#define ASSERT_NULL(_expr) \
    ASSERT_NULL_MSG(_expr, "Value is not NULL")
