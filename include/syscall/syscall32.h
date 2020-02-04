#pragma once

#include <stdint.h>
#include <cpu/core.h>
#include <generated/autoconf.h>

int syscall(int sysno, spctx_t *ctx, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5);
