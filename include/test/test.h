#pragma once

#include <log.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <utils/utils.h>
#include <time/core.h>
#include <generated/autoconf.h>

typedef enum {
    TEST_ERROR = -2,
    TEST_FAIL,
    TEST_PASS,
    TEST_SKIP,
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
    test_descriptor_t * _test_desc_ ## _test ## _ptr __attribute__ ((section (".test"))) = &_test_desc_ ## _test; \
    static testres_t _test(void)

#define TEND \
        return TEST_PASS;\
    }

/**
 * Defines a test case which requires the system.img to be of type <_fstype> and
 * mounted at /sys
 *
 * @param _fstype: Kind of file system
 * @param _test: Test name
 */
#define SYSIMG_T(_fstype, _test) \
    T(_test) { \
        if (!file_is_dir("/sys") || strncmp(CONFIG_FS_SYSTEM_IMAGE_TYPE, #_fstype, 10) != 0) { \
            return TEST_SKIP; \
        }

#define SYSIMG_TEND \
        } \
        return TEST_PASS;\
    }

/**
 * Defines a test case which requires the data.img to be of type <_fstype> and
 * mounted at /data
 *
 * @param _fstype: Kind of file system
 * @param _test: Test name
 */
#define DATAIMG_T(_fstype, _test) \
    T(_test) { \
        if (!file_is_dir("/data") || strncmp(CONFIG_FS_DATA_IMAGE_TYPE, #_fstype, 10) != 0) { \
            return TEST_SKIP; \
        }

#define DATAIMG_TEND \
        } \
        return TEST_PASS;\
    }

#define tassert(_expr) do { \
        __typeof__ (_expr) __exprres = (_expr); \
        if (!__exprres) { \
            error("[ " #_expr " ] failed in " __FILE__ ":" TOSTRING(__LINE__)); \
            return TEST_FAIL; \
        } \
    } while(0)

#define TEST_BUSY_WAIT_WHILE(_expr, _max_secs) do { \
        time_t _t; \
        time_get_rtc_time(&_t); \
        _t.secs += (_max_secs); \
        while (_expr) { \
            time_t _cur; \
            time_get_rtc_time(&_cur); \
            if (_cur.secs >= _t.secs) { \
                break; \
            } \
        } \
    } while(0)

#define TEST_BUSY_WAIT(_max_secs) TEST_BUSY_WAIT_WHILE(true, _max_secs)
