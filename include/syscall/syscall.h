#pragma once

#include <syscall/syscall-no.h>
#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall/syscall32.h>
#else
#include <syscall/syscall64.h>
#endif

void syscall_exit(int status);
int syscall_yield(void);
int syscall_puts(const char *str);
int syscall_getpid(void);
