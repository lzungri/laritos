#include <string.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *buf, int c, size_t count) {
    char *b = buf;
    while (count-- > 0) {
        *b++ = c;
    }
    return buf;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while(n-- > 0 && !(*s1 == '\0' && *s2 == '\0')) {
        int8_t diff = *s1++ - *s2++;
        if (diff) {
            return diff;
        }
    }
    return 0;
}
