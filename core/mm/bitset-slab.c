#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <sync/spinlock.h>
#include <dstruct/bitset.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>


typedef struct {
    spinlock_t lock;
    size_t elem_size;
    uint32_t bs_elems;
    uint32_t total_elems;
    uint32_t avail_elems;
    char *data;
    bitset_t bitset[];
} bs_slab_t;


static int totalelems_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    bs_slab_t *s = f->data0;

    irqctx_t ctx;
    spinlock_acquire(&s->lock, &ctx);
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", s->total_elems);
    spinlock_release(&s->lock, &ctx);

    return pseudofs_write_to_buf(buf, blen, data, strlen, offset);
}

static int avail_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    bs_slab_t *s = f->data0;

    irqctx_t ctx;
    spinlock_acquire(&s->lock, &ctx);
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", s->avail_elems);
    spinlock_release(&s->lock, &ctx);

    return pseudofs_write_to_buf(buf, blen, data, strlen, offset);
}

static inline int sysfs_create(bs_slab_t *slab) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%p", slab);
    fs_dentry_t *dir = vfs_dir_create(_laritos.fs.slab_root, buf,
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating slab sysfs directory");
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "totalelems", totalelems_read, slab) == NULL) {
        error("Failed to create 'totalelems' sysfs file");
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "avail", avail_read, slab) == NULL) {
        error("Failed to create 'avail' sysfs file");
        return -1;
    }

    return 0;
}

static inline int sysfs_remove(bs_slab_t *slab) {
    char buf[CONFIG_FS_MAX_FILENAME_LEN];
    snprintf(buf, sizeof(buf), "%p", slab);
    return vfs_dir_remove(_laritos.fs.slab_root, buf);
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

    sysfs_create(slab);

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
    sysfs_remove(slab);
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

static int slab_create_sysfs(fs_sysfs_mod_t *sysfs) {
    _laritos.fs.slab_root = vfs_dir_create(_laritos.fs.mem_root, "slab",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.slab_root == NULL) {
        error("Error creating slab sysfs directory");
        return -1;
    }
    return 0;
}

static int slab_remove_sysfs(fs_sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.mem_root, "slab");
}


SYSFS_MODULE(bitset_slab, slab_create_sysfs, slab_remove_sysfs)



#ifdef CONFIG_TEST_CORE_MM_BITSET_SLAB
#include __FILE__
#endif
