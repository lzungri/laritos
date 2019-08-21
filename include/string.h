#pragma once

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *buf, int c, size_t count);

int strncmp(const char *s1, const char *s2, size_t n);
