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

#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <printf.h>
#include <dstruct/circbuf.h>
#include <component/component.h>
#include <component/stream.h>
#include <component/bytestream.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>

static int bytestream_read(stream_t *s, void *buf, size_t n, bool blocking) {
    bytestream_t *bs = (bytestream_t *) s;
    return circbuf_read(&bs->rxcb, buf, n, blocking);
}

static int bytestream_write(struct stream *s, const void *buf, size_t n, bool blocking) {
    if (n == 0) {
        return 0;
    }
    bytestream_t *bs = (bytestream_t *) s;
    int ret;
    if ((ret = circbuf_write(&bs->txcb, buf, n, blocking)) <= 0) {
        return ret;
    }
    bs->ops.transmit_data(bs);
    return ret;
}

static int bytestream_put(bytestream_t *bs, const void *buf, size_t n) {
    return circbuf_nb_write(&bs->rxcb, buf, n);
}

int bytestream_component_init(bytestream_t *bs, board_comp_t *bcomp,
        void *rxbuf, size_t rxsize, void *txbuf, size_t txsize,
        int (*transmit_data)(bytestream_t *s)) {
    stream_t *s = (stream_t *) bs;
    char id[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(id, sizeof(id), "bytestream@%s", bcomp->id);
    if (stream_component_init(s, bcomp, id, COMP_TYPE_BYTESTREAM, bytestream_read, bytestream_write) < 0) {
        error("Failed to initialize '%s' stream component", id);
        return -1;
    }

    bs->ops._put = bytestream_put;
    bs->ops.transmit_data = transmit_data;

    circbuf_init(&bs->rxcb, rxbuf, rxsize);
    circbuf_init(&bs->txcb, txbuf, txsize);
    return 0;
}

int bytestream_component_register(bytestream_t *bs) {
    if (stream_component_register((stream_t *) bs) < 0) {
        error("Couldn't register '%s'", bs->parent.parent.id);
        return -1;
    }
    return 0;
}
