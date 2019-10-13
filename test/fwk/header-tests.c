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
