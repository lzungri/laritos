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
#include <fixedp.h>

T(q11_5_macro_creates_the_right_fixed_point_value) {
    q11_5_t v1 = Q11_5(5, 12345);
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 12345);

    v1 = Q11_5(5, 0);
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 0);

    v1 = Q11_5(0, 0);
    tassert(Q11_5_INT(v1) == 0);
    tassert(Q11_5_FRAC(v1) == 0);
TEND

T(q11_5_macro_creates_the_right_fixed_point_neg_value) {
    q11_5_t v1 = Q11_5(-5, 12345);
    tassert(Q11_5_INT(v1) == -5);
    tassert(Q11_5_FRAC(v1) == 12345);

    v1 = Q11_5(-5, 0);
    tassert(Q11_5_INT(v1) == -5);
    tassert(Q11_5_FRAC(v1) == 0);
TEND

T(q11_5_equality_test_works_as_expected) {
    q11_5_t v1 = Q11_5(5, 12345);
    tassert(v1 == Q11_5(5, 12345));
    tassert(v1 != Q11_5(5, 1234));
    tassert(v1 != Q11_5(5, 123));
    tassert(v1 != Q11_5(5, 12));
    tassert(v1 != Q11_5(5, 1));
    tassert(v1 != Q11_5(5, 0));

    tassert(Q11_5(0, 0) == Q11_5(0, 0));
TEND

T(q11_5_gt_test_works_as_expected) {
    tassert(Q11_5(2, 50) > Q11_5(1, 25));
    tassert(Q11_5(2, 50) > Q11_5(-1, 25));
    tassert(Q11_5(-2, 50) > Q11_5(-2, 75));
    tassert(Q11_5(-2, 50) > Q11_5(-5, 75));
    tassert(Q11_5(1, 0) > Q11_5(0, 0));
    tassert(Q11_5(-1, 0) > Q11_5(-2, 0));
TEND

T(q11_5_sum_returns_the_sum_of_the_two_fp_values) {
    q11_5_t v1 = Q11_5(1, 25);
    q11_5_t v2 = Q11_5(2, 50);
    q11_5_t v3 = q11_5_add(v1, v2);
    tassert(v3 == Q11_5(3, 75));
    tassert(Q11_5_INT(v3) == 3);
    tassert(Q11_5_FRAC(v3) == 75);
TEND
