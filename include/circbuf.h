#pragma once

#include <stdint.h>

typedef struct {
    uint8_t *buf;
    uint32_t size;
    uint32_t head;
    uint32_t datalen;
} circbuf_t;

#define DEF_STATIC_CIRCBUF(_name, _buf, _size) \
    static circbuf_t _name = { \
        .buf = (uint8_t *) (_buf), \
        .size = (_size), \
        .head = 0, \
    }

int circbuf_init(circbuf_t *cb, void *buf, uint32_t size);
int circbuf_write(circbuf_t *cb, void *buf, size_t n);
int circbuf_read(circbuf_t *cb, void *buf, size_t n);
