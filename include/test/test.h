#pragma once

#include <log.h>
#include <stdint.h>
#include <string.h>

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
#define DEF_TEST(_test) \
    static testres_t _test(void); \
    static test_descriptor_t _test_desc_ ## _test = { \
        .name = #_test, \
        .fpath = __FILE__, \
        .func = (_test), \
    }; \
    test_descriptor_t * _testdesc_ ## _test ## _ptr __attribute__ ((section (".test"))) = &_test_desc_ ## _test; \
    static testres_t _test(void)

#define DEF_TEST_END \
        return TEST_PASS;\
    }

#define assert_str_equals_msg(_expr, _v, _size, _msg, ...) do { \
        char *__exprc = (_expr); \
        char *__vc = (_v); \
        if (strncmp(__exprc, __vc, _size) != 0) { \
            error("'%s' != '%s': " _msg, __exprc, __vc, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define assert_str_equals(_expr, _v, _size) \
    assert_str_equals_msg(_expr, _v, _size, "Strings are not equal")

#define assert_eq_msg(_expr, _v, _msg, ...) do { \
        __typeof__ (_expr) __exprc = (_expr); \
        __typeof__ (_v) __vc = (_v); \
        if (__exprc != __vc) { \
            error(_msg, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define assert_eq(_expr, _v) \
    assert_eq_msg(_expr, _v, "Values are not equal")

#define assert_gt_msg(_expr, _v, _msg, ...) do { \
        __typeof__ (_expr) __exprc = (_expr); \
        __typeof__ (_v) __vc = (_v); \
        if (__exprc < __vc) { \
            error("%d should be greater than %d: "_msg, __exprc, __vc, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define assert_gt(_expr, _v) \
    assert_gt_msg(_expr, _v, "Value is not greater")

#define assert_lt_msg(_expr, _v, _msg, ...) do { \
        __typeof__ (_expr) __exprc = (_expr); \
        __typeof__ (_v) __vc = (_v); \
        if (__exprc > __vc) { \
            error("%d should be lower than %d: "_msg, __exprc, __vc, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define assert_lt(_expr, _v) \
    assert_lt_msg(_expr, _v, "Value is not greater")

#define assert_true_msg(_expr, _msg, ...) do { \
        if (!(_expr)) { \
            error(_msg, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define assert_true(_expr) \
    assert_true_msg(_expr, "Expression is not true")

#define assert_null_msg(_expr, _msg, ...) \
    assert_eq_msg(_expr, NULL, _msg, ##__VA_ARGS__)

#define assert_null(_expr) \
    assert_null_msg(_expr, "Value is not NULL")
