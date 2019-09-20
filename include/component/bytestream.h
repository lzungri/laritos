#pragma once

#include <stdbool.h>
#include <component/stream.h>
#include <dstruct/circbuf.h>

struct bytestream;
typedef struct {
    int (*_put)(struct bytestream *bs, const void *buf, size_t n);
    int (*_transmit)(struct bytestream *bs, const void *buf, size_t n);
    void (*_tx_ready)(struct bytestream *s);
    void (*_tx_notready)(struct bytestream *s);
} bytestream_ops_t;

typedef struct bytestream {
    stream_t parent;

    bytestream_ops_t ops;
    circbuf_t cb;

    // TODO Implement this as a synchro condition
    bool txready;
} bytestream_t;

int bytestream_component_init(bytestream_t *bs, board_comp_t *bcomp, void *buf, size_t size,
        int (*transmit)(bytestream_t *s, const void *buf, size_t n));
