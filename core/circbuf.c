#include <circbuf.h>
#include <string.h>
#include <math-utils.h>

int circbuf_write(circbuf_t *cb, void *buf, size_t n) {
    // TODO Check boundaries and limits
    if (n > cb->len) {
        n = cb->len;
    }

    char *endptr = (char *) cb->buf + cb->len;

    // Write the right area
    uint16_t nbytes_right = min(n, endptr - (char *) cb->wptr);
    memcpy(cb->wptr, buf, nbytes_right);

    // Write the left area
    int16_t nbytes_left = n - nbytes_right;
    memcpy(cb->buf, (char *) buf + nbytes_right, nbytes_left);

    // Update pointers
    if (nbytes_left > 0) {
        cb->wptr = (char *) cb->buf + nbytes_left;
        if (cb->wptr > cb->rptr) {
            cb->rptr = cb->wptr;
        }
    } else {
        char *new_wptr = (char *) cb->wptr + nbytes_right;
        if (cb->wptr < cb->rptr && new_wptr > (char *) cb->rptr) {
            cb->rptr = new_wptr;
        }
        cb->wptr = new_wptr;
    }

    return n;
}
