#pragma once

#include <log.h>

#define assert(_cond, _msg, ...) do { \
        if (!(_cond)) fatal(_msg, ##__VA_ARGS__); \
    } while(0)
