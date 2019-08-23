#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *buf;
    uint16_t len;
    void *wptr;
    void *rptr;
} circbuf_t;

#define DEF_STATIC_CIRCBUF(_name, _buf, _len) \
    static circbuf_t _name = { \
        .buf = _buf, \
        .len = _len, \
        .wptr = _buf, \
        .rptr = _buf, \
    }

int circbuf_write(circbuf_t *cb, void *buf, size_t n);
