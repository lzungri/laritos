//#define DEBUG
#include <log.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dstruct/list.h>
#include <mm/heap.h>
#include <utils/utils.h>
#include <utils/math.h>

// TODO Try to lock in time-sensitive funcs, then lock in the merge func
// TODO Lock!

typedef struct {
    struct list_head list;
    uint32_t size;
    char data[];
} fl_node_t;


/**
 * List of free blocks.
 * Ascending order to simplify the merging process
 */
static LIST_HEAD(freelist);


#ifdef DEBUG
static inline void dump_freelist(void) {
    fl_node_t *pos = NULL;
    log_always("Free list:");
    list_for_each_entry(pos, &freelist, list) {
        log_always("   [0x%p, size=%lu]", pos, pos->size);
    }
    heap_dump_info();
}
#endif

int heap_initialize(void *start, uint32_t size) {
    fl_node_t *whole = (fl_node_t *) start;
    whole->size = size - sizeof(fl_node_t);
    INIT_LIST_HEAD(&whole->list);
    list_add(&whole->list, &freelist);
#ifdef DEBUG
    dump_freelist();
#endif
    return 0;
}

void *malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    verbose("malloc(%d)", size);

    fl_node_t *best = NULL;
    fl_node_t *pos = NULL;
    list_for_each_entry(pos, &freelist, list) {
        if (pos->size >= size) {
            best = pos;
            verbose("Block found [0x%p, size=%lu]", pos, pos->size);

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
                verbose("Splitting: [0x%p, size=%lu] + [0x%p, size=%lu]", pos, pos->size, split, split->size);
            }

            list_del(&pos->list);

            break;
        }
    }

#ifdef DEBUG
    dump_freelist();
#endif

    return best != NULL ? best->data : NULL;
}

static void merge(void) {
    fl_node_t *pos;
    fl_node_t *tmp;

    verbose("Merging free blocks");
    list_for_each_entry_safe_reverse(pos, tmp, &freelist, list) {
        if (pos->list.prev == &freelist) {
            // First node, nothing else to merge
            break;
        }

        fl_node_t *prev = list_entry(pos->list.prev, fl_node_t, list);
        if ((char *) pos - prev->size == prev->data) {
            verbose("Merging: [0x%p, size=%lu] U [0x%p, size=%lu]", prev, prev->size, pos, pos->size);
            prev->size += pos->size + sizeof(fl_node_t);
            list_del(&pos->list);
        }
    }

#ifdef DEBUG
    dump_freelist();
#endif
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    fl_node_t *node = container_of(ptr, fl_node_t, data);
    verbose("Freeing ptr=0x%p, block [0x%p, size=%lu]", ptr, node, node->size);

    struct list_head *pos;
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
}

void *calloc(size_t nmemb, size_t size) {
    // Very basic implementation
    size_t n = size * nmemb;
    void *ptr = malloc(n);
    if (ptr != NULL) {
        memset(ptr, 0, n);
    }
    return ptr;
}

uint32_t heap_get_available(void) {
    uint32_t avail = 0;
    fl_node_t *block;
    list_for_each_entry(block, &freelist, list) {
        avail += block->size;
    }
    return avail;
}

void heap_dump_info(void) {
    uint32_t blocks = 0;
    uint32_t maxs = 0;
    uint32_t mins = 0xffffffff;
    fl_node_t *block;
    list_for_each_entry(block, &freelist, list) {
        blocks++;
        maxs = max(maxs, block->size);
        mins = min(mins, block->size);
    }
    log_always("Heap information:");
    log_always("  Available: %lu bytes", heap_get_available());
    log_always("  Number of free blocks: %lu", blocks);
    log_always("  Max free block size: %lu", maxs);
    log_always("  Min free block size: %lu", blocks > 0 ? mins : 0);
}



#ifdef CONFIG_TEST_CORE_MM_FREELIST
#include __FILE__
#endif
