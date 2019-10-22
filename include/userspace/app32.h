#pragma once

#include <stdint.h>

/**
 * App's magic number ("LARO")
 */
#define APPMAGIC 0x4F52414c

#define HEADER_VERSION 0

// TODO Remove this, use ELF directly
typedef struct {
    uint32_t magic;
    uint32_t hdrver;

    uint32_t app_size;

    uint32_t text_start;
    uint32_t text_size;
    uint32_t data_start;
    uint32_t data_size;
    uint32_t got_start;
    uint32_t got_size;
    uint32_t reloc_start;
    uint32_t reloc_size;
    uint32_t symbol_start;
    uint32_t symbol_size;
    uint32_t bss_start;
    uint32_t bss_size;
    uint32_t heap_start;
    uint32_t heap_size;
    uint32_t stack_end;
    uint32_t stack_size;
} appheader_t;
