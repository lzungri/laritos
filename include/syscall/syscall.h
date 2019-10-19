#pragma once

#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall/syscall32.h>
#else
#include <syscall/syscall64.h>
#endif
