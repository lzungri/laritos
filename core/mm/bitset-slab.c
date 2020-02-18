#include <log.h>

#include <stdbool.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <sync/spinlock.h>
#include <dstruct/bitset.h>


typedef struct {
    spinlock_t lock;
    size_t elem_size;
    uint32_t bs_elems;
    uint32_t total_elems;
    uint32_t avail_elems;
    char *data;
    bitset_t bitset[];
} bs_slab_t;


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

    verbose_async("slab=0x%p new", slab);

    return (slab_t *) slab;
}

void *slab_alloc(slab_t *slab) {
    if (slab == NULL) {
        return NULL;
    }

    bs_slab_t *s = (bs_slab_t *) slab;

    irqctx_t ctx;
    spinlock_acquire(&s->lock, &ctx);

    if (slab_get_avail_elems(slab) == 0) {
        spinlock_release(&s->lock, &ctx);
        return NULL;
    }

    uint32_t idx = bitset_array_ffz(s->bitset, s->bs_elems);
    if (idx == BITSET_ARRAY_IDX_NOT_FOUND) {
        spinlock_release(&s->lock, &ctx);
        return NULL;
    }

    verbose_async("slab=0x%p alloc #%lu", slab, idx);

    bitset_array_lm_set(s->bitset, s->bs_elems, idx);
    s->avail_elems--;

    spinlock_release(&s->lock, &ctx);

    return slab_get_ptr_from_position(slab, idx);
}

void slab_free(slab_t *slab, void *ptr) {
    if (slab == NULL || ptr == NULL) {
        return;
    }

    bs_slab_t *s = (bs_slab_t *) slab;
    uint32_t bsidx = ((char *) ptr - s->data) / s->elem_size;

    irqctx_t ctx;
    spinlock_acquire(&s->lock, &ctx);

    // Check if it belongs to a real block and if it is actually being used
    if (bsidx < s->total_elems && slab_is_taken(s, bsidx)) {
        bitset_array_lm_clear(s->bitset, s->bs_elems, bsidx);
        s->avail_elems++;
    }

    spinlock_release(&s->lock, &ctx);

    verbose_async("slab=0x%p free #%lu", slab, bsidx);
}

void slab_destroy(slab_t *slab) {
    if (slab == NULL) {
        return;
    }
    free(slab);
    verbose_async("slab=0x%p destroy", slab);
}

uint32_t slab_get_avail_elems(slab_t *slab) {
    return ((bs_slab_t *) slab)->avail_elems;
}

uint32_t slab_get_total_elems(slab_t *slab) {
    return ((bs_slab_t *) slab)->total_elems;
}

uint32_t slab_get_slab_position(slab_t *slab, void *ptr) {
    bs_slab_t *s = (bs_slab_t *) slab;
    return ((char *) ptr - s->data) / s->elem_size;
}

void *slab_get_ptr_from_position(slab_t *slab, uint32_t pos) {
    bs_slab_t *s = (bs_slab_t *) slab;
    return (void *) (s->data + pos * s->elem_size);
}

bool slab_is_taken(slab_t *slab, uint32_t idx) {
    bs_slab_t *s = (bs_slab_t *) slab;
    return bitset_array_lm_bit(s->bitset, s->bs_elems, idx) != 0;
}


#ifdef CONFIG_TEST_CORE_MM_BITSET_SLAB
#include __FILE__
#endif
