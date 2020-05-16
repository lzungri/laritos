/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * The goal of this file is to #include (and therefore compile) all the
 * tests targeting header functionality.
 *
 * Since including a source file in a header may cause compilation errors
 * or duplicate test cases, we need to do it manually here.
 */
#include <log.h>

#include <generated/autoconf.h>

#ifdef CONFIG_TEST_CORE_UTILS_LATENCY
#include <core/utils/latency.c>
#endif

#ifdef CONFIG_TEST_CORE_DSTRUCT_BITSET
#include <core/dstruct/bitset.c>
#endif

#ifdef CONFIG_TEST_CORE_SYNC_ATOMIC
#include <core/sync/atomic.c>
#endif

#ifdef CONFIG_TEST_CORE_COMPONENT_CPU
#include <core/component/cpu.c>
#endif

#ifdef CONFIG_TEST_CORE_UTILS_MATH
#include <core/utils/math.c>
#endif

#ifdef CONFIG_TEST_CORE_LIBC_FIXEDP
#include <core/libc/fixedp.c>
#endif
