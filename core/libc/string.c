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

int memcmp(const void *buf1, const void *buf2, size_t n) {
    const char *p1 = buf1;
    const char *p2 = buf2;
    while(n-- > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        } else {
            p1++;
            p2++;
        }
    }
    return 0;
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
    char *match = NULL;
    char *ss = (char *) substr;

    if (*ss == '\0') {
        return (char *) &str[strnlen(str, n)];
    }

    while (n-- > 0 && *str != '\0' && *ss != '\0') {
        if (*str == *ss) {
            if (match == NULL) {
                match = (char *) str;
            }
            ss++;
        } else if (match != NULL) {
            match = NULL;
            ss = (char *) substr;
        }
        str++;
    }
    return *ss == '\0' ? match : NULL;
}
