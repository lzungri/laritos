#pragma once

#include <stdint.h>

/**
 * App's magic number ("LARO")
 */
#define APPMAGIC 0x4F52414c

typedef struct {
    uint32_t magic;

    uint32_t app_size;

    void *text_start;
    void *text_size;
    void *data_start;
    void *data_size;
    void *got_start;
    void *got_size;
    void *bss_start;
    void *bss_size;
    void *reloc_start;
    void *reloc_size;
    void *heap_start;
    void *heap_size;
    void *stack_end;
    void *stack_size;
} appheader_t;
