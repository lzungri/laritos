#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sync/spinlock.h>
#include <sync/condition.h>

typedef struct {
    uint8_t *buf;
    uint32_t size;
    uint32_t head;
    uint32_t datalen;
    spinlock_t lock;
    condition_t data_avail_cond;
    condition_t space_avail_cond;
    irqctx_t peek_ctx;
    size_t peek_size;
} circbuf_t;

#define DEF_STATIC_CIRCBUF(_name, _buf, _size) \
    static circbuf_t _name = { \
        .buf = (uint8_t *) (_buf), \
        .size = (_size), \
        .head = 0, \
        .data_avail_cond = CONDITION_STATIC_INIT(_name.data_avail_cond), \
        .space_avail_cond = CONDITION_STATIC_INIT(_name.space_avail_cond), \
    }

int circbuf_write(circbuf_t *cb, const void *buf, size_t n, bool blocking);
int circbuf_nb_write(circbuf_t *cb, const void *buf, size_t n);
int circbuf_read(circbuf_t *cb, void *buf, size_t n, bool blocking);
int circbuf_nb_read(circbuf_t *cb, void *buf, size_t n);
int circbuf_peek(circbuf_t *cb, void *buf, size_t n);
int circbuf_peek_complete(circbuf_t *cb, bool commit);

static inline int circbuf_init(circbuf_t *cb, void *buf, uint32_t size) {
    cb->buf = (uint8_t *) buf;
    cb->head = 0;
    cb->datalen = 0;
    cb->size = size;
    spinlock_init(&cb->lock);
    condition_init(&cb->data_avail_cond);
    condition_init(&cb->space_avail_cond);
    return 0;
}

static inline int circbuf_reset(circbuf_t *cb) {
    cb->head = 0;
    cb->datalen = 0;
    return 0;
}

static inline uint32_t circbuf_get_datalen(circbuf_t *cb) {
    return cb->datalen;
}

static inline bool circbuf_is_empty(circbuf_t *cb) {
    return cb->datalen == 0;
}

static inline bool circbuf_is_full(circbuf_t *cb) {
    return cb->datalen == cb->size;
}
