#pragma once

#include <log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_ASSERT_PRINT_MSG
#define assert(_cond, _msg, ...) do { \
        if (!(_cond)) { \
            error("[ " #_cond " ] failed in " __FILE__ ":" TOSTRING(__LINE__)); \
            fatal(_msg, ##__VA_ARGS__); \
        } \
    } while(0)
#else
#define assert(_cond, _msg, ...) do { \
        if (!(_cond)) { \
            while (1); \
        } \
    } while(0)
#endif
