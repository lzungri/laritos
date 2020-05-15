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
#include <test/test.h>
#include <math.h>

T(math_sign_extend_16_32_preserves_sign) {
    tassert(sign_extend_32(0xFFFF, 16) == 0xFFFFFFFF);
    tassert(sign_extend_32(0x8001, 16) == 0xFFFF8001);
    tassert(sign_extend_32(0x1, 16) == 0x1);
    tassert(sign_extend_32(0x7FFF, 16) == 0x00007FFF);
    tassert(sign_extend_32(0x8FFF, 16) == 0xFFFF8FFF);
TEND

T(math_sign_extend_24_32_preserves_sign) {
    tassert(sign_extend_32(0xFFFFFF, 24) == 0xFFFFFFFF);
    tassert(sign_extend_32(0x800001, 24) == 0xFF800001);
    tassert(sign_extend_32(0x1, 24) == 0x1);
    tassert(sign_extend_32(0x7FFFFF, 24) == 0x007FFFFF);
    tassert(sign_extend_32(0x8FFFFF, 24) == 0xFF8FFFFF);
TEND
