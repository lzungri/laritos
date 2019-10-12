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

T(bitset_ffz_returns_negative_when_all_1) {
    bitset_t bs;
    memset(&bs, 0xff, sizeof(bitset_t));
    tassert(bitset_ffz(bs) < 0);
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
