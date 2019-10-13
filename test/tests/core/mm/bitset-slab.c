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
        tassert(is_slab_avail(slab, i) == false);
    }
    tassert(slab_get_avail_elems(slab) == 0);

    for (i = slab->total_elems - 1; i >= 0; i--) {
        slab_free(slab, v[i]);
        tassert(slab_get_avail_elems(slab) == slab->total_elems - i);
        tassert(is_slab_avail(slab, i) == true);
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
