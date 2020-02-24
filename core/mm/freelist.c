#include <log.h>

#include <stddef.h>
#include <stdint.h>
#include <dstruct/list.h>
#include <mm/heap.h>
#include <utils/utils.h>
#include <utils/math.h>
#include <sync/spinlock.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>

typedef struct {
    list_head_t list;
    uint32_t size;
    char data[];
} fl_node_t;

/**
 * List of free blocks.
 * Ascending order to simplify the merging process
 */
static LIST_HEAD(freelist);

/**
 * Freelist lock
 */
static spinlock_t lock;


#ifdef DEBUG
static inline void dump_freelist(void) {
    fl_node_t *pos = NULL;
    log_always_async("Free list:");
    list_for_each_entry(pos, &freelist, list) {
        log_always_async("   [0x%p, size=%lu]", pos, pos->size);
    }
}
#endif

int heap_initialize(void *start, uint32_t size) {
    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    fl_node_t *whole = (fl_node_t *) start;
    whole->size = size - sizeof(fl_node_t);
    INIT_LIST_HEAD(&whole->list);
    list_add(&whole->list, &freelist);

    spinlock_release(&lock, &ctx);
    return 0;
}

void *_malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    insane_async("malloc(%d)", size);

    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    fl_node_t *best = NULL;
    fl_node_t *pos = NULL;
    list_for_each_entry(pos, &freelist, list) {
        if (pos->size >= size) {
            best = pos;
            insane_async("Block found [0x%p, size=%lu]", pos, pos->size);

            size_t size_and_meta = size + sizeof(fl_node_t);

            // Check whether we need to split this chunk
            if (pos->size > size_and_meta) {
                fl_node_t *split = (fl_node_t *) ((char *) pos + size_and_meta);
                split->size = pos->size - size_and_meta;
                INIT_LIST_HEAD(&split->list);
                list_add(&split->list, &freelist);

                // We only update the size of the selected chunk if it was split.
                // If we don't have any available space for splitting the chunk, we just keep
                // the original size even if we waste some memory
                pos->size = size;
                insane_async("Splitting: [0x%p, size=%lu] + [0x%p, size=%lu]", pos, pos->size, split, split->size);
            }

            list_del(&pos->list);

            break;
        }
    }

#ifdef DEBUG
    dump_freelist();
#endif

    spinlock_release(&lock, &ctx);

    return best != NULL ? best->data : NULL;
}

static void merge(void) {
    fl_node_t *pos;
    fl_node_t *tmp;

    insane_async("Merging free blocks");
    list_for_each_entry_safe_reverse(pos, tmp, &freelist, list) {
        if (pos->list.prev == &freelist) {
            // First node, nothing else to merge
            break;
        }

        fl_node_t *prev = list_entry(pos->list.prev, fl_node_t, list);
        if ((char *) pos - prev->size == prev->data) {
            insane_async("Merging: [0x%p, size=%lu] U [0x%p, size=%lu]", prev, prev->size, pos, pos->size);
            prev->size += pos->size + sizeof(fl_node_t);
            list_del(&pos->list);
        }
    }

#ifdef DEBUG
    dump_freelist();
#endif
}

void _free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    fl_node_t *node = container_of(ptr, fl_node_t, data);
    insane_async("Freeing ptr=0x%p, block [0x%p, size=%lu]", ptr, node, node->size);

    list_head_t *pos;
    list_for_each(pos, &freelist) {
        if (pos > &node->list) {
            __list_add(&node->list, pos->prev, pos);
            goto end;
        }
    }

    list_add_tail(&node->list, &freelist);

end:
#ifdef DEBUG
    dump_freelist();
#endif
    merge();

    spinlock_release(&lock, &ctx);
}

uint32_t heap_get_available(void) {
    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    uint32_t avail = 0;
    fl_node_t *block;
    list_for_each_entry(block, &freelist, list) {
        avail += block->size;
    }

    spinlock_release(&lock, &ctx);
    return avail;
}

static int status_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    uint32_t blocks = 0;
    uint32_t maxs = 0;
    uint32_t mins = 0xffffffff;
    fl_node_t *block;
    list_for_each_entry(block, &freelist, list) {
        blocks++;
        maxs = max(maxs, block->size);
        mins = min(mins, block->size);
    }

    spinlock_release(&lock, &ctx);

    char data[512];
    int strlen = snprintf(data, sizeof(data),
            "Free blocks: %lu\n"
            "Max size: %lu\n"
            "Min size: %lu",
            blocks, maxs, blocks > 0 ? mins : 0);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int freeblocks_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    irqctx_t ctx;
    spinlock_acquire(&lock, &ctx);

    char data[512];
    uint32_t totalb = 0;
    fl_node_t *pos = NULL;
    list_for_each_entry(pos, &freelist, list) {
        int strlen = snprintf(data + totalb, sizeof(data) - totalb, "0x%p %lu\n", pos, pos->size);
        if (strlen < 0) {
            spinlock_release(&lock, &ctx);
            return -1;
        }
        totalb += strlen;
    }

    spinlock_release(&lock, &ctx);

    return pseudofs_write_to_buf(buf, blen, data, totalb, offset);
}

static int freelist_create_sysfs(sysfs_mod_t *sysfs) {
    fs_dentry_t *dir = vfs_dir_create(_laritos.fs.mem_root, "freelist",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating freelist sysfs directory");
        return -1;
    }

    if (pseudofs_create_custom_ro_file(dir, "status", status_read) == NULL) {
        error("Failed to create 'status' sysfs file");
    }

    if (pseudofs_create_custom_ro_file(dir, "freeblocks", freeblocks_read) == NULL) {
        error("Failed to create 'freeblocks' sysfs file");
    }

    return 0;
}

static int freelist_remove_sysfs(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.mem_root, "freelist");
}


SYSFS_MODULE(freelist, freelist_create_sysfs, freelist_remove_sysfs)


#ifdef CONFIG_TEST_CORE_MM_FREELIST
#include __FILE__
#endif
