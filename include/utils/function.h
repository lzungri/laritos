#pragma once

#include <log.h>

#define DEF_NOT_IMPL_FUNC(_func, ...) \
    static int _func(__VA_ARGS__) { \
        error_async(TOSTRING(_func) "() not Implemented"); \
        return -1; \
    }
