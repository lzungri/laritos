#pragma once

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *buf, int c, size_t count);
int memcmp(const void *buf1, const void *buf2, size_t n);

int strncmp(const char *s1, const char *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
size_t strlen(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
char *strchr(const char *s, char c);
char *strrchr(const char *s, char c);
char *strstr(const char *str, const char *substr, size_t n);
char *strncat(char *s1, const char *s2, size_t n);
