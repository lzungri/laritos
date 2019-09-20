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
    if (s->blocking) {
        return circbuf_blocking_read(&bs->cb, buf, n);
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

static void bytestream_txready_update(bytestream_t *bs, bool ready) {
    verbose_async("Transmit line is %sready", ready ? "" : "not ");
    bs->txready = ready;
}

static int bytestream_transmit_nop(bytestream_t *s, const void *buf, size_t n) {
    return -1;
}

int bytestream_component_init(bytestream_t *bs, board_comp_t *bcomp, void *buf, size_t size,
        int (*transmit)(bytestream_t *s, const void *buf, size_t n)) {
    stream_t *s = (stream_t *) bs;
    char id[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(id, sizeof(id), "bytestream@%s", bcomp->id);
    if (stream_component_init(s, bcomp, id, bytestream_read, bytestream_write) < 0) {
        error("Failed to initialize '%s' stream component", id);
        return -1;
    }
    ((component_t *) bs)->stype = COMP_SUBTYPE_BYTESTREAM;

    bs->ops._put = bytestream_put;
    bs->ops._tx_ready_update = bytestream_txready_update;
    bs->ops._transmit = transmit != NULL ? transmit : bytestream_transmit_nop;

    return circbuf_init(&bs->cb, buf, size);
}
