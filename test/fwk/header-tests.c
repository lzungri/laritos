/**
 * The goal of this file is to #include (and therefore compile) all the
 * tests targeting header functionality.
 *
 * Since including a source file in a header may cause compilation errors
 * or duplicate test cases, we need to do it manually here.
 */
#include <log.h>

#include <generated/autoconf.h>

#ifdef CONFIG_TEST_CORE_DSTRUCT_BITSET
#include <core/dstruct/bitset.c>
#endif

#ifdef CONFIG_TEST_CORE_SYNC_ATOMIC
#include <core/sync/atomic.c>
#endif

#ifdef CONFIG_TEST_CORE_COMPONENT_CPU
#include <core/component/cpu.c>
#endif

#ifdef CONFIG_TEST_UTILS_MATH
#include <utils/math.c>
#endif

#ifdef CONFIG_TEST_UTILS_LATENCY
#include <utils/latency.c>
#endif
