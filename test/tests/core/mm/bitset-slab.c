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
#include <mm/slab.h>

T(bitsetslab_returns_null_when_cannot_satisfy_mem_requirement) {
    slab_t *slab = slab_create(1, sizeof(uint32_t));
    tassert(slab != NULL);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    uint32_t *v2 = slab_alloc(slab);
    tassert(v2 == NULL);
    v2 = slab_alloc(slab);
    tassert(v2 == NULL);

    slab_free(slab, v1);

    v2 = slab_alloc(slab);
    tassert(v2 != NULL);
    slab_free(slab, v2);

    slab_destroy(slab);
TEND

T(bitsetslab_cannot_use_a_destroyed_slab) {
    slab_t *slab = slab_create(1, sizeof(uint32_t));
    tassert(slab != NULL);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    slab_destroy(slab);

    v1 = slab_alloc(slab);
    tassert(v1 == NULL);
TEND

T(bitsetslab_freeing_or_destroying_null_slab_doesnt_crash_os) {
    slab_t *slab = slab_create(1, sizeof(uint32_t));
    tassert(slab != NULL);
    slab_free(slab, NULL);
    slab_destroy(slab);
    slab_free(NULL, NULL);
    slab_destroy(NULL);
TEND

T(bitsetslab_cannot_create_empty_slab) {
    slab_t *slab = slab_create(0, sizeof(uint32_t));
    tassert(slab == NULL);
    slab_destroy(slab);
    slab = slab_create(1, 0);
    tassert(slab == NULL);
    slab_destroy(slab);
TEND

T(bitsetslab_returns_correct_available_items) {
    slab_t *slab = slab_create(3, sizeof(uint32_t));
    tassert(slab != NULL);
    tassert(slab_get_avail_elems(slab) == 3);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    tassert(slab_get_avail_elems(slab) == 2);
    uint32_t *v2 = slab_alloc(slab);
    tassert(v2 != NULL);
    tassert(slab_get_avail_elems(slab) == 1);
    uint32_t *v3 = slab_alloc(slab);
    tassert(v3 != NULL);
    tassert(slab_get_avail_elems(slab) == 0);
    uint32_t *v4 = slab_alloc(slab);
    tassert(v4 == NULL);
    tassert(slab_get_avail_elems(slab) == 0);

    slab_free(slab, v2);
    tassert(slab_get_avail_elems(slab) == 1);
    slab_free(slab, v1);
    slab_free(slab, v3);
    tassert(slab_get_avail_elems(slab) == 3);

    slab_destroy(slab);
TEND

T(bitsetslab_freeing_already_freed_slab_ignores_the_request) {
    slab_t *slab = slab_create(2, sizeof(uint32_t));
    tassert(slab != NULL);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    uint32_t *v2 = slab_alloc(slab);
    tassert(v2 != NULL);

    slab_free(slab, v1);
    tassert(slab_get_avail_elems(slab) == 1);
    slab_free(slab, v2);
    tassert(slab_get_avail_elems(slab) == 2);
    slab_free(slab, v1);
    tassert(slab_get_avail_elems(slab) == 2);

    slab_destroy(slab);
TEND

T(bitsetslab_allocating_last_avail_slab_returns_always_the_same_slabptr) {
    slab_t *slab = slab_create(2, sizeof(uint32_t));
    tassert(slab != NULL);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    uint32_t *v2 = slab_alloc(slab);
    tassert(v2 != NULL);

    slab_free(slab, v1);
    tassert(slab_get_avail_elems(slab) == 1);

    uint32_t *v3 = slab_alloc(slab);
    tassert(v3 != NULL);
    tassert(v3 == v1);

    slab_destroy(slab);
TEND

T(bitsetslab_large_num_of_allocs) {
    bs_slab_t *slab = (bs_slab_t *) slab_create(100, sizeof(uint32_t));
    tassert(slab != NULL);

    int i;
    uint32_t *v[100];
    for (i = 0; i < slab->total_elems; i++) {
        v[i] = slab_alloc(slab);
        tassert(v[i] != NULL);
        tassert((char *) v[i] == slab->data + i * sizeof(uint32_t));
        tassert(slab_get_avail_elems(slab) == slab->total_elems - i - 1);
        tassert(slab_is_taken(slab, i));
    }
    tassert(slab_get_avail_elems(slab) == 0);

    for (i = slab->total_elems - 1; i >= 0; i--) {
        slab_free(slab, v[i]);
        tassert(slab_get_avail_elems(slab) == slab->total_elems - i);
        tassert(!slab_is_taken(slab, i));
    }
    tassert(slab_get_avail_elems(slab) == slab->total_elems);

    slab_destroy(slab);
TEND

T(bitsetslab_freeing_an_invalid_ptr_ignores_the_request) {
    slab_t *slab = slab_create(1, sizeof(uint32_t));
    tassert(slab != NULL);

    uint32_t *v1 = slab_alloc(slab);
    tassert(v1 != NULL);
    tassert(slab_get_avail_elems(slab) == 0);
    slab_free(slab, v1 + 10000);
    tassert(slab_get_avail_elems(slab) == 0);
    slab_free(slab, v1);
    tassert(slab_get_avail_elems(slab) == 1);

    slab_destroy(slab);
TEND

T(bitsetslab_cannot_allocate_slab_with_MAXUINT32_elements) {
    slab_t *slab = slab_create(0xFFFFFFFF, sizeof(uint32_t));
    tassert(slab == NULL);
    slab_destroy(slab);
TEND

T(bitsetslab_slab_has_the_right_amount_of_bsarray_elements) {
    bs_slab_t *slab = (bs_slab_t *) slab_create(BITSET_NBITS, sizeof(uint32_t));
    tassert(slab != NULL);
    tassert(slab->bs_elems == 1);
    slab_destroy(slab);

    slab = (bs_slab_t *) slab_create(BITSET_NBITS + 1, sizeof(uint32_t));
    tassert(slab != NULL);
    tassert(slab->bs_elems == 2);
    slab_destroy(slab);

    slab = (bs_slab_t *) slab_create(BITSET_NBITS * 3 + 1, sizeof(uint32_t));
    tassert(slab != NULL);
    tassert(slab->bs_elems == 4);
    slab_destroy(slab);
TEND

T(bitsetslab_slab_returns_the_right_slab_position) {
    bs_slab_t *slab = (bs_slab_t *) slab_create(10, sizeof(char));
    tassert(slab != NULL);
    tassert(slab_get_avail_elems(slab) == 10);
    int i;
    for (i = 0; i < 10; i++) {
        char *v1 = slab_alloc(slab);
        tassert(slab_get_slab_position(slab, v1) == i);
    }
    slab_destroy(slab);
TEND

T(bitsetslab_get_ptr_returns_null_on_invalid_position) {
    bs_slab_t *slab = (bs_slab_t *) slab_create(10, sizeof(char));
    tassert(slab != NULL);
    tassert(slab_get_avail_elems(slab) == 10);
    tassert(slab_get_ptr_from_position(slab, 0) != NULL);
    tassert(slab_get_ptr_from_position(slab, 11) == NULL);
    tassert(slab_get_ptr_from_position(slab, 110) == NULL);
    slab_destroy(slab);
TEND

T(bitsetslab_get_position_returns_null_on_invalid_position) {
    bs_slab_t *slab = (bs_slab_t *) slab_create(10, sizeof(char));
    tassert(slab != NULL);
    tassert(slab_get_avail_elems(slab) == 10);
    tassert(slab_get_slab_position(slab, slab_get_ptr_from_position(slab, 9)) == 9);
    tassert(slab_get_slab_position(slab, slab_get_ptr_from_position(slab, 9) + 1) == -1);
    slab_destroy(slab);
TEND
