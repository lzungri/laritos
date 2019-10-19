#pragma once

#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <userspace/app32.h>
#else
#include <userspace/app64.h>
#endif
