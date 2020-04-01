/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdint.h>
#include <utils/math.h>

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

int memcmp(const void *buf1, const void *buf2, size_t n) {
    const char *p1 = buf1;
    const char *p2 = buf2;
    while (n-- > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- > 0 && !(*s1 == '\0' && *s2 == '\0')) {
        int8_t diff = *s1++ - *s2++;
        if (diff) {
            return diff;
        }
    }
    return 0;
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t size;
    for (size = 0; *s++ != '\0' && maxlen-- > 0; size++);
    return size;
}


size_t strlen(const char *s) {
    size_t size;
    for (size = 0; *s++ != '\0'; size++);
    return size;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strchr(const char *str, char c) {
    while (*str != c) {
        if (!*str++) {
            return NULL;
        }
    }
    return (char *) str;
}

char *strrchr(const char *s, char c) {
    char *ret = NULL;
    do {
        if (*s == c) {
            ret = (char *) s;
        }
    } while (*s++);
    return ret;
}

char *strstr(const char *str, const char *substr, size_t n) {
    if (*substr == '\0') {
        return (char *) &str[strnlen(str, n)];
    }

    n = min(n, strlen(substr));
    while (*str != '\0') {
        if (memcmp(str, substr, n) == 0) {
            return (char *) str;
        }
        str++;
    }
    return NULL;
}

char *strncat(char *s1, const char *s2, size_t n) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    if (len2 < n) {
        strncpy(&s1[len1], s2, n);
    } else {
        strncpy(&s1[len1], s2, n);
        s1[len1 + n] = '\0';
    }
    return s1;
}



#ifdef CONFIG_TEST_CORE_LIBC_STRING
#include __FILE__
#endif
