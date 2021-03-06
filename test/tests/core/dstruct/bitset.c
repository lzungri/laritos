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
#include <string.h>
#include <test/test.h>
#include <dstruct/bitset.h>

T(bitset_cannot_set_beyond_the_num_bits) {
    bitset_t bs = 0;
    tassert(bitset_lm_set(&bs, BITSET_NBITS + 1) < 0);
    tassert(bitset_lm_set(&bs, BITSET_NBITS + 100) < 0);
    tassert(bitset_lm_clear(&bs, BITSET_NBITS + 1) < 0);
    tassert(bitset_lm_clear(&bs, BITSET_NBITS + 100) < 0);
TEND

T(bitset_lm_set_sets_the_right_bit) {
    int i;
    for (i = 0; i < BITSET_NBITS; i++) {
        bitset_t bs = 0;
        bitset_lm_set(&bs, i);
        tassert(bs & (1 << (BITSET_NBITS - i - 1)));
        tassert((bs & ~(1 << (BITSET_NBITS - i - 1))) == 0);
    }
TEND

T(bitset_lm_clear_clears_the_right_bit) {
    int i;
    for (i = 0; i < BITSET_NBITS; i++) {
        bitset_t bs;
        memset(&bs, 0xff, sizeof(bitset_t));
        bitset_lm_clear(&bs, i);
        tassert((bs & (1 << (BITSET_NBITS - i - 1))) == 0);
        tassert(bs & ~(1 << (BITSET_NBITS - i - 1)));
    }
TEND

T(bitset_ffz_returns_BITSET_IDX_NOT_FOUND_when_all_1) {
    bitset_t bs;
    memset(&bs, 0xff, sizeof(bitset_t));
    tassert(bitset_ffz(bs) == BITSET_IDX_NOT_FOUND);
TEND

T(bitset_ffz_returns_the_right_pos_on_first_zero) {
    int i;
    for (i = 0; i < BITSET_NBITS; i++) {
        bitset_t bs;
        memset(&bs, 0xff, sizeof(bitset_t));
        bitset_lm_clear(&bs, i);
        tassert(bitset_ffz(bs) == i);
    }
TEND

T(bitset_lm_bit_returns_the_right_bit_value) {
    bitset_t bs = 0;
    int i;
    for (i = 0; i < BITSET_NBITS; i++) {
        tassert(bitset_lm_bit(bs, i) == 0);
    }
    memset(&bs, 0xff, sizeof(bitset_t));
    for (i = 0; i < BITSET_NBITS; i++) {
        tassert(bitset_lm_bit(bs, i) == 1);
    }
TEND
