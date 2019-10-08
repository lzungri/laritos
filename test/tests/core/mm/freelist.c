#include <log.h>

#include <stdint.h>
#include <test/test.h>

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

