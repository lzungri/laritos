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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <dstruct/circbuf.h>
#include <sync/spinlock.h>
#include <sync/condition.h>

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

    irqctx_t ctx;
    spinlock_acquire(&cb->lock, &ctx);
    if (blocking) {
        BLOCK_UNTIL(n <= cb->size - cb->datalen, &cb->space_avail_cond, &cb->lock, &ctx);
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

    // Notify readers there is some data available to be read
    condition_notify_all_locked(&cb->data_avail_cond);

    spinlock_release(&cb->lock, &ctx);

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

    irqctx_t ctx;
    spinlock_acquire(&cb->lock, &ctx);
    if (blocking) {
        // Wait until there is some data in the buffer
        BLOCK_UNTIL(cb->datalen > 0, &cb->data_avail_cond, &cb->lock, &ctx);
    }

    if (n > cb->datalen) {
        n = cb->datalen;
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
        spinlock_release(&cb->lock, &ctx);
        // Notify writers there is some space available for storing data
        condition_notify_all_locked(&cb->space_avail_cond);
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
        // Notify writers there is some space available for storing data
        condition_notify_all_locked(&cb->space_avail_cond);
    }
    spinlock_release(&cb->lock, &cb->peek_ctx);
    return 0;
}



#ifdef CONFIG_TEST_CORE_DSTRUCT_CIRCBUF
#include __FILE__
#endif
