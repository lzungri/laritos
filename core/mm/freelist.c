#include <log.h>

#include <stddef.h>
#include <string.h>
#include <dstruct/list.h>
#include <mm/heap.h>
#include <utils/utils.h>

// TODO Keep track of available heap
// TODO Try to lock in time-sensitive funcs, then lock in the merge func
// TODO Lock!
// TODO Merge

typedef struct {
    struct list_head list;
    size_t size;
    char data[];
} fl_node_t;


LIST_HEAD(freelist);

int initialize_heap(void *start, size_t size) {
    fl_node_t *whole = (fl_node_t *) start;
    whole->size = size - sizeof(fl_node_t);
    INIT_LIST_HEAD(&whole->list);
    list_add(&whole->list, &freelist);
    return 0;
}

void *malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    fl_node_t *best = NULL;
    fl_node_t *pos = NULL;
    fl_node_t *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &freelist, list) {
        if (pos->size >= size) {
            best = pos;

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
            }

            list_del(&pos->list);

            break;
        }
    }

    return best != NULL ? best->data : NULL;
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    fl_node_t *node = container_of(ptr, fl_node_t, data);

    struct list_head *pos;
    list_for_each(pos, &freelist) {
        if (pos > &node->list) {
            __list_add(&node->list, pos->prev, pos);
            return;
        }
    }

    list_add_tail(&node->list, &freelist);
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
