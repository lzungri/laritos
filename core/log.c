#include <log.h>
#include <circbuf.h>
#include <stdarg.h>
#include <utils.h>
#include <printf.h>

static char logb[CONFIG_LOG_BUFSIZE_BYTES];

DEF_STATIC_CIRCBUF(logcb, logb, sizeof(logb));


int __add_log_msg(char *level, char *tag, char *fmt, ...) {
    char buf[CONFIG_LOG_MAX_LINE_SIZE] = { 0 };

    int nchars = snprintf(buf, sizeof(buf), "%s:%s:  ", level, tag);
    // If the required number of chars is bigger than the size of the buffer, then truncate string
    if (nchars > sizeof(buf)) {
        goto full_buf;
    }

    va_list ap;
    va_start(ap, fmt);
    int nchars2 = vsnprintf(buf + nchars, sizeof(buf) - nchars, fmt, ap);
    va_end(ap);
    // If the required number of chars is bigger than the remaining buffer, then truncate string
    if (nchars2 > sizeof(buf) - nchars) {
        goto full_buf;
    }

    return circbuf_write(&logcb, buf, nchars + nchars2);

full_buf:
    buf[sizeof(buf) - 1] = '\n';
    return circbuf_write(&logcb, buf, sizeof(buf));
}

int log_flush(void) {
    // TODO Kconfig log transport (e.g. uart)
    return 0;
}
