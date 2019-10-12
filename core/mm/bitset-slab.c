#include <log.h>

#include <stdbool.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <dstruct/bitset.h>


typedef struct {
    size_t elem_size;
    uint32_t bs_elems;
    uint32_t total_elems;
    uint32_t avail_elems;
    char *data;
    bitset_t bitset[];
} bs_slab_t;


static inline bool is_slab_avail(bs_slab_t *s, uint8_t idx) {
    return bitset_array_lm_bit(s->bitset, s->bs_elems, idx) == 0;
}

slab_t *slab_create(uint32_t numelems, size_t elemsize) {
    if (numelems == 0 || numelems == BITSET_ARRAY_IDX_NOT_FOUND || elemsize == 0) {
        return NULL;
    }

    // Space for the struct
    size_t size = sizeof(bs_slab_t);

    // Space for the bitset array
    uint32_t bselems = numelems / BITSET_NBITS;
    bselems += (numelems % BITSET_NBITS > 0) ? 1 : 0;
    size += bselems * sizeof(bitset_t);

    // Offset to the data chunk
    size_t dataoffset = size;
    // Space for the actual slabs
    size += numelems * elemsize;

    bs_slab_t *slab = calloc(1, size);
    if (slab == NULL) {
        return NULL;
    }
    slab->bs_elems = bselems;
    slab->total_elems = numelems;
    slab->avail_elems = numelems;
    slab->elem_size = elemsize;
    slab->data = (char *) slab + dataoffset;

    return (slab_t *) slab;
}

void *slab_alloc(slab_t *slab) {
    if (slab == NULL || slab_get_avail_elems(slab) == 0) {
        return NULL;
    }

    bs_slab_t *s = (bs_slab_t *) slab;
    uint32_t idx = bitset_array_ffz(s->bitset, s->bs_elems);
    if (idx == BITSET_ARRAY_IDX_NOT_FOUND) {
        return NULL;
    }

    bitset_array_lm_set(s->bitset, s->bs_elems, idx);
    s->avail_elems--;
    return s->data + idx * s->elem_size;
}

void slab_free(slab_t *slab, void *ptr) {
    if (slab == NULL || ptr == NULL) {
        return;
    }

    bs_slab_t *s = (bs_slab_t *) slab;
    uint32_t bsidx = ((char *) ptr - s->data) / s->elem_size;

    // Check if it belongs to a real block and if it is actually being used
    if (bsidx < s->total_elems && !is_slab_avail(s, bsidx)) {
        bitset_array_lm_clear(s->bitset, s->bs_elems, bsidx);
        s->avail_elems++;
    }
}

void slab_destroy(slab_t *slab) {
    if (slab == NULL) {
        return;
    }
    free(slab);
}

uint32_t slab_get_avail_elems(slab_t *slab) {
    return ((bs_slab_t *) slab)->avail_elems;
}



#ifdef CONFIG_TEST_CORE_MM_BITSET_SLAB
#include __FILE__
#endif
