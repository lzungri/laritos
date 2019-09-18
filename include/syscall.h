#pragma once

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall32.h>
#else
#include <syscall64.h>
#endif

int syscall(int sysno, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5);
