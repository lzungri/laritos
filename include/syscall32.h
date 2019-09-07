#pragma once

#include <stdint.h>
#include <generated/autoconf.h>

typedef struct {
    int32_t p[CONFIG_SYSCALL_MAX_ARGS];
} syscall_params_t;
