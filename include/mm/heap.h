#pragma once

#include <stddef.h>
#include <stdint.h>
#include <generated/autoconf.h>

// Start address of the OS heap
extern char __heap_start[];
// End address of the OS heap
extern char __heap_end[];

int heap_initialize(void *start, uint32_t size);
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
uint32_t heap_get_available(void);
void heap_dump_info(void);
