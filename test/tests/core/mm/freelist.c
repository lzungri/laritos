#include <log.h>

#include <stdint.h>
#include <test/test.h>
#include <dstruct/list.h>

static int get_num_blocks(void) {
    fl_node_t *block;
    uint32_t blocks = 0;
    list_for_each_entry(block, &freelist, list) {
        blocks++;
    }
    return blocks;
}

T(freelist_heap_reports_the_right_available_space) {
    uint32_t avail = heap_get_available();

    char *p = malloc(0);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = malloc(10);
    tassert(heap_get_available() == avail - sizeof(fl_node_t) - 10);
    free(p);
    tassert(heap_get_available() == avail);

    p = malloc(heap_get_available());
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
TEND

T(freelist_malloc_returns_null_when_cannot_satisfy_mem_req) {
    uint32_t avail = heap_get_available();

    char *p = malloc(avail + 1);
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = malloc(avail);
    tassert(p != NULL);
    tassert(heap_get_available() == 0);
    char *p2 = malloc(1);
    tassert(p2 == NULL);
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
    p2 = malloc(1);
    tassert(p != NULL);
    tassert(heap_get_available() == avail - 1 - sizeof(fl_node_t));
TEND

T(freelist_free_merges_adjacent_free_blocks) {
    uint32_t avail = heap_get_available();
    uint32_t blocks = get_num_blocks();

    char *p[3];

    p[0] = malloc(10);
    tassert(p[0] != NULL);
    // A block is removed from the list and a new one is added
    tassert(get_num_blocks() == blocks);

    p[1] = malloc(10);
    tassert(p[1] != NULL);
    tassert(get_num_blocks() == blocks);

    p[2] = malloc(10);
    tassert(p[2] != NULL);
    tassert(get_num_blocks() == blocks);

    free(p[0]);
    tassert(get_num_blocks() == blocks + 1);
    free(p[2]);
    tassert(get_num_blocks() == blocks + 1);
    free(p[1]);
    tassert(get_num_blocks() == blocks);
    tassert(heap_get_available() == avail);
TEND

T(freelist_heap_reports_the_right_available_space_with_calloc) {
    uint32_t avail = heap_get_available();

    char *p = calloc(0, 0);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = calloc(1, 10);
    tassert(heap_get_available() == avail - sizeof(fl_node_t) - 10);
    free(p);
    tassert(heap_get_available() == avail);

    p = calloc(1, heap_get_available());
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
TEND

T(freelist_calloc_returns_null_when_cannot_satisfy_mem_req) {
    uint32_t avail = heap_get_available();

    char *p = calloc(1, avail + 1);
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = calloc(1, avail);
    tassert(p != NULL);
    tassert(heap_get_available() == 0);
    char *p2 = calloc(1, 1);
    tassert(p2 == NULL);
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
    p2 = calloc(1, 1);
    tassert(p != NULL);
    tassert(heap_get_available() == avail - 1 - sizeof(fl_node_t));
TEND

T(freelist_calloc_returns_a_chunk_filled_with_0s) {
    char *p = calloc(1, 100);
    tassert(p != NULL);
    int i;
    for (i = 0; i < 100; i++) {
        tassert(p[i] == 0);
    }
    free(p);
TEND

T(freelist_calloc_and_free_merges_adjacent_free_blocks) {
    uint32_t avail = heap_get_available();
    uint32_t blocks = get_num_blocks();

    char *p[3];

    p[0] = calloc(1, 10);
    tassert(p[0] != NULL);
    // A block is removed from the list and a new one is added
    tassert(get_num_blocks() == blocks);

    p[1] = calloc(1, 10);
    tassert(p[1] != NULL);
    tassert(get_num_blocks() == blocks);

    p[2] = calloc(1, 10);
    tassert(p[2] != NULL);
    tassert(get_num_blocks() == blocks);

    free(p[0]);
    tassert(get_num_blocks() == blocks + 1);
    free(p[2]);
    tassert(get_num_blocks() == blocks + 1);
    free(p[1]);
    tassert(get_num_blocks() == blocks);
    tassert(heap_get_available() == avail);
TEND
