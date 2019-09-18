#pragma once

#include <stdint.h>

typedef struct {
    void *buf;
    uint32_t size;
    void *wptr;
    void *rptr;
    uint32_t datalen;
} circbuf_t;

#define DEF_STATIC_CIRCBUF(_name, _buf, _size) \
    static circbuf_t _name = { \
        .buf = (_buf), \
        .size = (_size), \
        .wptr = (_buf), \
        .rptr = (_buf), \
    }

int circbuf_init(circbuf_t *cb, void *buf, uint32_t size);
int circbuf_write(circbuf_t *cb, void *buf, size_t n);
int circbuf_read(circbuf_t *cb, void *buf, size_t n);
