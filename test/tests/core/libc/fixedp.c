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

T(q11_5_macro_rounds_up_to_nearest_fraction) {
    q11_5_t v1 = Q11_5(5, 0); // 5.0
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 0);

    v1 = Q11_5(5, 1); // 5.00001
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 3125);

    v1 = Q11_5(5, 6000); // 5.06
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 6250);

    v1 = Q11_5(5, 33250); // 5.33250
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 34375);

    v1 = Q11_5(5, 95750);
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 96875);

    v1 = Q11_5(5, 99875);
    tassert(Q11_5_INT(v1) == 5);
    tassert(Q11_5_FRAC(v1) == 96875);
TEND

T(q11_5_macro_creates_the_right_fixed_point_value) {
    int i;
    q11_5_t v1;
    for (i = 0; i < 1024; i++) {
        v1 = Q11_5(i, 12345);
        tassert(Q11_5_INT(v1) == i);
        tassert(Q11_5_FRAC(v1) == 12500);
    }

    v1 = Q11_5(0, 0);
    tassert(Q11_5_INT(v1) == 0);
    tassert(Q11_5_FRAC(v1) == 0);
TEND

T(q11_5_macro_support_min_max_values) {
    q11_5_t v1 = Q11_5(1023, 99999);
    tassert(Q11_5_INT(v1) == 1023);
    tassert(Q11_5_FRAC(v1) == 96875);

    v1 = Q11_5(-1024, 0);
    tassert(Q11_5_INT(v1) == -1024);
    tassert(Q11_5_FRAC(v1) == 0);
TEND

T(q11_5_macro_creates_the_right_fixed_point_neg_value) {
    int i;
    q11_5_t v1;
    for (i = -1024; i < 0; i++) {
        v1 = Q11_5(i, 0);
        tassert(Q11_5_INT(v1) == i);
        tassert(Q11_5_FRAC(v1) == 0);
    }

    v1 = Q11_5(-5, 0);
    tassert(Q11_5_INT(v1) == -5);
    tassert(Q11_5_FRAC(v1) == 0);
TEND

T(q11_5_equality_test_works_as_expected) {
    q11_5_t v1 = Q11_5(5, 12345);
    tassert(v1 == Q11_5(5, 12500));
    tassert(v1 != Q11_5(5, 1250));
    tassert(v1 != Q11_5(5, 125));
    tassert(v1 != Q11_5(5, 12));
    tassert(v1 != Q11_5(5, 1));
    tassert(v1 != Q11_5(5, 0));

    tassert(Q11_5(0, 0) == Q11_5(0, 0));

    tassert(Q11_5(0, 50000) == Q11_5(0, 50000));
    tassert(Q11_5(0, 123) == Q11_5(0, 3125));
    tassert(Q11_5(0, 80000) == Q11_5(0, 81250));
    tassert(Q11_5(0, 9000) == Q11_5(0, 9375));
TEND

T(q11_5_gt_test_works_as_expected) {
    tassert(Q11_5(2, 50) > Q11_5(1, 25));
    tassert(Q11_5(2, 50) > Q11_5(-1, 25));
    tassert(Q11_5(-2, 10) > Q11_5(-2, 9999));
    tassert(Q11_5(-2, 50) > Q11_5(-5, 50));
    tassert(Q11_5(1, 0) > Q11_5(0, 0));
    tassert(Q11_5(-1, 0) > Q11_5(-2, 0));
    tassert(Q11_5(-1, 0) > Q11_5(-1, 1));
TEND

T(q11_5_sum_returns_the_sum_of_the_two_fp_values) {
    q11_5_t v1 = Q11_5(1, 9375);
    q11_5_t v2 = Q11_5(2, 15625);
    q11_5_t v3 = q11_5_add(v1, v2);
    tassert(v3 == Q11_5(3, 25000));
    tassert(Q11_5_INT(v3) == 3);
    tassert(Q11_5_FRAC(v3) == 25000);
TEND

T(q11_5_sum_returns_the_sum_of_the_two_fp_values2) {
    tassert(q11_5_add(Q11_5(0, 0), Q11_5(0, 0)) == Q11_5(0, 0));
    tassert(q11_5_add(Q11_5(0, 0), Q11_5(0, 43750)) == Q11_5(0, 43750));
    tassert(q11_5_add(Q11_5(1, 0), Q11_5(12345, 0)) == Q11_5(12346, 0));
TEND

T(q11_5_sum_saturates_to_16bits) {
    tassert(q11_5_add(Q11_5(1023, 99999), Q11_5(1, 0)) == Q11_5(1023, 96875));
    tassert(q11_5_add(Q11_5(1000, 0), Q11_5(100, 0)) == Q11_5(1023, 96875));
TEND

T(q11_5_sum_neg_values_saturates_to_16bits) {
    tassert(q11_5_add(Q11_5(-1000, 0), Q11_5(-100, 0)) == Q11_5(-1024, 0));
    tassert(q11_5_add(Q11_5(-1000, 0), Q11_5(-1000, 0)) == Q11_5(-1024, 0));
TEND

T(q11_5_sum_carries_fractional_part) {
    tassert(q11_5_add(Q11_5(1, 50000), Q11_5(1, 50000)) == Q11_5(3, 0));
    tassert(q11_5_add(Q11_5(1, 59375), Q11_5(0, 62500)) == Q11_5(2, 21875));
    tassert(q11_5_add(Q11_5(1, 96875), Q11_5(12345, 96875)) == Q11_5(12347, 93750));
    tassert(q11_5_add(Q11_5(-1, 50000), Q11_5(-100, 50000)) == Q11_5(-102, 0));
TEND

T(q11_5_sum_neg_returns_the_sum_of_the_two_fp_values) {
    tassert(q11_5_add(Q11_5(0, 0), Q11_5(-1, 0)) == Q11_5(-1, 0));
    tassert(q11_5_add(Q11_5(-1, 12500), Q11_5(-1, 12500)) == Q11_5(-2, 25000));
    tassert(q11_5_add(Q11_5(-1, 125), Q11_5(1, 0)) == -Q11_5(0, 125));
TEND

T(q11_5_sub_returns_the_substraction_of_the_two_fp_values) {
    tassert(q11_5_sub(Q11_5(0, 0), Q11_5(1, 0)) == Q11_5(-1, 0));
    tassert(q11_5_sub(Q11_5(-1, 12500), Q11_5(1, 12500)) == Q11_5(-2, 25000));
    tassert(q11_5_sub(Q11_5(-1, 125), Q11_5(-1, 0)) == -Q11_5(0, 125));
TEND

T(q11_5_sub_carries_fractional_part) {
    tassert(q11_5_sub(Q11_5(-1, 50000), Q11_5(100, 50000)) == Q11_5(-102, 0));
TEND

T(q11_5_mul_returns_the_multiplication_of_the_two_fp_values) {
    tassert(q11_5_mul(Q11_5(0, 0), Q11_5(1, 0)) == Q11_5(0, 0));
    tassert(q11_5_mul(Q11_5(0, 0), Q11_5(1, 99999)) == Q11_5(0, 0));
    tassert(q11_5_mul(Q11_5(1, 0), Q11_5(1, 0)) == Q11_5(1, 0));
    tassert(q11_5_mul(Q11_5(2, 0), Q11_5(1, 0)) == Q11_5(2, 0));
    tassert(q11_5_mul(Q11_5(2, 0), Q11_5(2, 0)) == Q11_5(4, 0));
    tassert(q11_5_mul(Q11_5(2, 50000), Q11_5(2, 0)) == Q11_5(5, 0));
    tassert(q11_5_mul(Q11_5(3, 12500), Q11_5(2, 0)) == Q11_5(6, 25000));
    tassert(q11_5_mul(Q11_5(3, 50000), Q11_5(2, 0)) == Q11_5(7, 0));
    tassert(q11_5_mul(Q11_5(3, 50000), Q11_5(2, 50000)) == Q11_5(8, 75000));
    tassert(q11_5_mul(Q11_5(3, 15625), Q11_5(2, 3125)) == Q11_5(6, 40625));
    tassert(q11_5_mul(Q11_5(3, 15625), Q11_5(2, 28125)) == Q11_5(7, 18750));
    tassert(q11_5_mul(Q11_5(3, 50000), Q11_5(-2, 50000)) == Q11_5(-8, 75000));
    tassert(q11_5_mul(Q11_5(-3, 50000), Q11_5(2, 50000)) == Q11_5(-8, 75000));
    tassert(q11_5_mul(Q11_5(-3, 50000), Q11_5(-2, 50000)) == Q11_5(8, 75000));
TEND

T(q11_5_mul_saturates_to_16bits) {
    tassert(q11_5_mul(Q11_5(1023, 99999), Q11_5(2, 0)) == Q11_5(1023, 96875));
    tassert(q11_5_mul(Q11_5(1000, 0), Q11_5(100, 0)) == Q11_5(1023, 96875));
TEND

T(q11_5_mul_neg_value_saturates_to_16bits) {
    tassert(q11_5_mul(Q11_5(-1024, 0), Q11_5(2, 0)) == Q11_5(-1024, 0));
    tassert(q11_5_mul(Q11_5(-1024, 0), Q11_5(1000, 0)) == Q11_5(-1024, 0));
TEND
