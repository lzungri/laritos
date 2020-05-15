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

#include <log.h>

#include <core.h>
#include <random.h>
#include <utils/math.h>

void random_seed(uint32_t seed) {
    _laritos.rndseed = seed;
}

/**
 * Same LCG (Linear Congruential Generator) used by glicb, basic and exploitable.
 */
int32_t random_int32(int32_t min, int32_t max) {
    /**
     * X(n+1) = (X(n) * a + c) % m
     *
     * multiplier a = 1103515245
     * increment c  = 12345
     * modulus m    = 2^31
     */
    _laritos.rndseed = (_laritos.rndseed * 1103515245 + 12345) & 0x7fffffff;
    return _laritos.rndseed % (max - min + 1) + min;
}



#ifdef CONFIG_TEST_CORE_LIBC_RANDOM_LCG
#include __FILE__
#endif
