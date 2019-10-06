#pragma once

#include <stddef.h>
#include <generated/autoconf.h>

// Start address of the OS heap
extern char __heap_start[];
// End address of the OS heap
extern char __heap_end[];

int initialize_heap(void *start, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
