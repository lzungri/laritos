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

#include <stdint.h>
#include <fixedp.h>

/**
 * Map of Fractional binary value -> fraction value.
 * Useful for finding the fractional binary representation for a given
 * fractional number.
 */
const uint32_t _q11_5_fractions[] = {
        0, // 00000 -> <= .0
     3125, // 00001 -> <= .03125
     6250, // 00010 -> <= .0625
     9375,
    12500,
    15625,
    18750,
    21875,
    25000,
    28125,
    31250,
    34375,
    37500,
    40625,
    43750,
    46875,
    50000,
    53125,
    56250,
    59375,
    62500,
    65625,
    68750,
    71875,
    75000,
    78125,
    81250,
    84375,
    87500,
    90625,
    93750,
    96875, // <= .96875 -> 11111
};



#ifdef CONFIG_TEST_CORE_LIBC_FIXEDP
#include __FILE__
#endif
