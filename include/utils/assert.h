#pragma once

#include <log.h>

#define assert(_cond, _msg, ...) do { \
        if (!(_cond)) { \
            error("[ " #_cond " ] failed in " __FILE__ ":" TOSTRING(__LINE__)); \
            fatal(_msg, ##__VA_ARGS__); \
        } \
    } while(0)
