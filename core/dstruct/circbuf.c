#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sync/sync.h>
#include <utils/math.h>
#include <dstruct/circbuf.h>

int circbuf_write(circbuf_t *cb, const void *buf, size_t n, bool blocking) {
    if (cb == NULL || buf == NULL) {
        return -1;
    }

    if (n < 0) {
        return 0;
    }

    if (n > cb->size) {
        n = cb->size;
    }

    ctx_t ctx;
    if (blocking) {
        spinlock_ctx_save(&cb->lock, &ctx);
        while (n > cb->size - cb->datalen) {
            spinlock_ctx_restore(&cb->lock, &ctx);
            // TODO Implement a better sync mechanism here
            asm("wfi");
            spinlock_ctx_save(&cb->lock, &ctx);
        }
    } else {
        spinlock_ctx_save(&cb->lock, &ctx);
    }

    uint32_t windex = (cb->head + cb->datalen) % cb->size;
    uint32_t nbytes_right = min(n, cb->size - windex);
    // Write the right area
    memcpy(&cb->buf[windex], buf, nbytes_right);
    // Write the left area
    memcpy(cb->buf, (uint8_t *) buf + nbytes_right, n - nbytes_right);

    // Update the amount of data available for reading
    cb->datalen += n;
    if (cb->datalen > cb->size) {
        // Move the head in case we overwrote old non-read data
        cb->head = (cb->head + (cb->datalen - cb->size)) % cb->size;
        cb->datalen = cb->size;
    }

    spinlock_ctx_restore(&cb->lock, &ctx);

    return n;
}

int circbuf_nb_write(circbuf_t *cb, const void *buf, size_t n) {
    return circbuf_write(cb, buf, n, false);
}

static int do_circbuf_read(circbuf_t *cb, void *buf, size_t n, bool blocking, bool peek) {
    if (n < 0 || cb == NULL || buf == NULL) {
        return -1;
    }

    if (n < 0) {
        return 0;
    }

    if (n > cb->datalen) {
        n = cb->datalen;
    }

    ctx_t ctx;
    if (blocking) {
        spinlock_ctx_save(&cb->lock, &ctx);
        // Wait until there is some data in the buffer
        while (cb->datalen == 0) {
            spinlock_ctx_restore(&cb->lock, &ctx);
            // TODO Replace with a nice synchro mechanism
            // Any interrupt will wake the cpu up
            asm("wfi");
            spinlock_ctx_save(&cb->lock, &ctx);
        }
    } else {
        spinlock_ctx_save(&cb->lock, &ctx);
    }

    // Read the right area
    uint32_t nbytes_right = min(n, cb->size - cb->head);
    memcpy(buf, &cb->buf[cb->head], nbytes_right);
    // Read the left area
    memcpy((char *) buf + nbytes_right, cb->buf, n - nbytes_right);

    if (peek && n > 0) {
        cb->peek_ctx = ctx;
        cb->peek_size = n;
    } else {
        cb->head = (cb->head + n) % cb->size;
        cb->datalen -= n;
        spinlock_ctx_restore(&cb->lock, &ctx);
    }

    return n;
}

int circbuf_read(circbuf_t *cb, void *buf, size_t n, bool blocking) {
    return do_circbuf_read(cb, buf, n, blocking, false);
}

int circbuf_nb_read(circbuf_t *cb, void *buf, size_t n) {
    return do_circbuf_read(cb, buf, n, false, false);
}

int circbuf_peek(circbuf_t *cb, void *buf, size_t n) {
    return do_circbuf_read(cb, buf, n, false, true);
}

int circbuf_peek_complete(circbuf_t *cb, bool commit) {
    if (commit) {
        cb->head = (cb->head + cb->peek_size) % cb->size;
        cb->datalen -= cb->peek_size;
    }
    spinlock_ctx_restore(&cb->lock, &cb->peek_ctx);
    return 0;
}
