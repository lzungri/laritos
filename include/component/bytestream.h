#pragma once

#include <stdbool.h>
#include <component/stream.h>
#include <dstruct/circbuf.h>

struct bytestream;
typedef struct {
    int (*_put)(struct bytestream *bs, const void *buf, size_t n);
    int (*transmit_data)(struct bytestream *bs);
} bytestream_ops_t;

typedef struct bytestream {
    stream_t parent;

    bytestream_ops_t ops;
    circbuf_t rxcb;
    circbuf_t txcb;
} bytestream_t;

int bytestream_component_init(bytestream_t *bs, board_comp_t *bcomp,
        void *rxbuf, size_t rxsize, void *txbuf, size_t txsize,
        int (*transmit_data)(bytestream_t *bs));
