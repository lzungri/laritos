#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <printf.h>
#include <dstruct/circbuf.h>
#include <component/component.h>
#include <component/stream.h>
#include <component/bytestream.h>

static int bytestream_read(stream_t *s, void *buf, size_t n) {
    bytestream_t *bs = (bytestream_t *) s;
    // TODO Move this logic to circbuf_read
    if (s->blocking) {
        // Wait until there is some data in the buffer
        while (bs->cb.datalen == 0) {
            // TODO Replace with a nice synchro mechanism
            // Any interrupt (rx int is enabled) will wake the cpu up
            asm("wfi");
        }
    }
    return circbuf_read(&bs->cb, buf, n);
}

static int bytestream_write(struct stream *s, const void *buf, size_t n) {
    if (n == 0) {
        return 0;
    }

    bytestream_t *bs = (bytestream_t *) s;
    if (!s->blocking) {
        return bs->txready ? bs->ops._transmit(bs, buf, n) : 0;
    }

    int sent = 0;
    while (!bs->txready || (sent += bs->ops._transmit(bs, (uint8_t *) buf + sent, n - sent)) < n) {
        if (!bs->txready) {
            // TODO Replace with a nice synchro mechanism
            // Any interrupt will wake the cpu up
            asm("wfi");
        }
    }
    return sent;
}

static int bytestream_put(bytestream_t *bs, const void *buf, size_t n) {
    return circbuf_write(&bs->cb, buf, n);
}

static void bytestream_txready(bytestream_t *bs) {
    verbose_sync(false, "Transmit line is ready");
    bs->txready = true;
}

static void bytestream_txnotready(bytestream_t *bs) {
    verbose_sync(false, "Transmit line not ready");
    bs->txready = false;
}

static int bytestream_transmit_nop(bytestream_t *s, const void *buf, size_t n) {
    return -1;
}

int bytestream_component_init(bytestream_t *bs, board_comp_t *bcomp, void *buf, size_t size,
        int (*transmit)(bytestream_t *s, const void *buf, size_t n)) {
    ((component_t *) bs)->stype = COMP_SUBTYPE_BYTESTREAM;
    circbuf_init(&bs->cb, buf, size);
    bs->ops._put = bytestream_put;
    bs->ops._tx_ready = bytestream_txready;
    bs->ops._tx_notready = bytestream_txnotready;
    bs->ops._transmit = transmit != NULL ? transmit : bytestream_transmit_nop;

    char id[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(id, sizeof(id), "bytestream@%s", bcomp->id);
    return stream_component_init((stream_t *) bs, bcomp, id, NULL, NULL, bytestream_read, bytestream_write);
}
