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
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = malloc(10);
#ifdef CONFIG_MEM_HEAP_BUFFER_PROTECTION
    tassert(heap_get_available() <= avail - sizeof(fl_node_t) - 10);
#else
    tassert(heap_get_available() == avail - sizeof(fl_node_t) - 10);
#endif
    free(p);
    tassert(heap_get_available() == avail);

#ifndef CONFIG_MEM_HEAP_BUFFER_PROTECTION
    p = malloc(heap_get_available());
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
#endif
TEND

T(freelist_malloc_returns_null_when_cannot_satisfy_mem_req) {
    uint32_t avail = heap_get_available();

    char *p = malloc(avail + 1);
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

#ifndef CONFIG_MEM_HEAP_BUFFER_PROTECTION
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
#endif
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
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

    p = calloc(1, 10);
#ifdef CONFIG_MEM_HEAP_BUFFER_PROTECTION
    tassert(heap_get_available() <= avail - sizeof(fl_node_t) - 10);
#else
    tassert(heap_get_available() == avail - sizeof(fl_node_t) - 10);
#endif
    free(p);
    tassert(heap_get_available() == avail);

#ifndef CONFIG_MEM_HEAP_BUFFER_PROTECTION
    p = calloc(1, heap_get_available());
    tassert(heap_get_available() == 0);
    free(p);
    tassert(heap_get_available() == avail);
#endif
TEND

T(freelist_calloc_returns_null_when_cannot_satisfy_mem_req) {
    uint32_t avail = heap_get_available();

    char *p = calloc(1, avail + 1);
    tassert(p == NULL);
    tassert(heap_get_available() == avail);
    free(p);
    tassert(heap_get_available() == avail);

#ifndef CONFIG_MEM_HEAP_BUFFER_PROTECTION
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
#endif
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
