#include <stddef.h>
#include <string.h>
#include <math-utils.h>
#include <circbuf.h>

#include <stdint.h>

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

    char *endptr = (char *) cb->buf + cb->size;

    // Write the right area
    uint32_t nbytes_right = min(n, endptr - (char *) cb->wptr);
    memcpy(cb->wptr, buf, nbytes_right);

    // Write the left area
    uint32_t nbytes_left = n - nbytes_right;
    memcpy(cb->buf, (char *) buf + nbytes_right, nbytes_left);

    // Update pointers
    char *new_wptr;
    if (nbytes_left > 0) {
        new_wptr = (char *) cb->buf + nbytes_left;
        // Update the read pointer if the write pointer passes over it and there was data
        // available for reading
        if (new_wptr > (char *) cb->rptr || (cb->wptr <= cb->rptr && cb->datalen > 0)) {
            cb->rptr = new_wptr;
        }
        cb->wptr = new_wptr;
    } else {
        new_wptr = (char *) cb->wptr + nbytes_right;
        // Update read ptr only if there is some data left to read
        if ((cb->wptr <= cb->rptr && cb->datalen > 0) && new_wptr > (char *) cb->rptr) {
            cb->rptr = new_wptr;
        }
        cb->wptr = new_wptr;
    }


    // Update the amount of data available for reading
    cb->datalen += n;
    if (cb->datalen > cb->size) {
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

    if (cb->wptr > cb->rptr) {
        uint32_t nbytes = min(n, (char *) cb->wptr - (char *) cb->rptr);
        memcpy(buf, cb->rptr, nbytes);
        cb->rptr = (char *) cb->rptr + nbytes;
        cb->datalen -= nbytes;
        return nbytes;
    }

    char *endptr = (char *) cb->buf + cb->size;

    // Read the right area
    uint32_t nbytes_right = min(n, endptr - (char *) cb->rptr);
    memcpy(buf, cb->rptr, nbytes_right);

    // Read the left area
    uint32_t nbytes_left = min(n - nbytes_right, (char *) cb->wptr - (char *) cb->buf);
    memcpy((char *) buf + nbytes_right, cb->buf, nbytes_left);

    if (nbytes_left > 0) {
        cb->rptr = (char *) cb->buf + nbytes_left;
    } else {
        cb->rptr = (char *) cb->rptr + nbytes_right;
    }

    cb->datalen -= nbytes_left + nbytes_right;
    return nbytes_left + nbytes_right;
}
