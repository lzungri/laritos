#pragma once

#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall32.h>
#else
#include <syscall64.h>
#endif

int syscall(int sysno, syscall_params_t *params);
