#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <utils/math.h>
#include <circbuf.h>


int circbuf_write(circbuf_t *cb, void *buf, size_t n) {
    if (cb == NULL || buf == NULL) {
        return -1;
    }

    if (n < 0) {
        n = 0;
    }

    if (n > cb->size) {
        n = cb->size;
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

    return n;
}

int circbuf_read(circbuf_t *cb, void *buf, size_t n) {
    if (n < 0 || cb == NULL || buf == NULL) {
        return -1;
    }

    if (n < 0) {
        n = 0;
    }

    if (n > cb->datalen) {
        n = cb->datalen;
    }

    // Read the right area
    uint32_t nbytes_right = min(n, cb->size - cb->head);
    memcpy(buf, &cb->buf[cb->head], nbytes_right);
    // Read the left area
    memcpy((char *) buf + nbytes_right, cb->buf, n - nbytes_right);

    cb->head = (cb->head + n) % cb->size;
    cb->datalen -= n;

    return n;
}
