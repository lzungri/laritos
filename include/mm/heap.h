#pragma once

#include <log.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <generated/autoconf.h>

// Start address of the OS heap
extern char __heap_start[];
// End address of the OS heap
extern char __heap_end[];

int heap_initialize(void *start, uint32_t size);
uint32_t heap_get_available(void);
void heap_dump_info(void);
void *_malloc(size_t size);
void _free(void *ptr);

#ifdef CONFIG_MEM_HEAP_BUFFER_PROTECTION
#include <mm/_heap_prot.h>
#else
static inline void *malloc(size_t size) {
    return _malloc(size);
}

static inline void free(void *ptr) {
    _free(ptr);
}
#endif

__attribute__((always_inline)) static inline void *calloc(size_t nmemb, size_t size) {
    // Very basic implementation
    size_t n = size * nmemb;
    void *ptr = malloc(n);
    if (ptr != NULL) {
        memset(ptr, 0, n);
    }
    return ptr;
}
