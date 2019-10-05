#pragma once

#include <stddef.h>

int initialize_heap(void *start, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
