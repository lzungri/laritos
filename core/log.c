#include <log.h>
#include <stdarg.h>

#include <circbuf.h>
#include <printf.h>
#include <utils.h>

static char logb[CONFIG_LOG_BUFSIZE_BYTES];

DEF_STATIC_CIRCBUF(logcb, logb, sizeof(logb));


int __add_log_msg(char *level, char *tag, char *fmt, ...) {
    int ret = 0;
    char lineb[CONFIG_LOG_MAX_LINE_SIZE] = { 0 };

    int nchars = snprintf(lineb, sizeof(lineb), "%s:%s:", level, tag);
    // If the required number of chars is bigger than the size of the buffer, then truncate string
    if (nchars > sizeof(lineb)) {
        goto full_buf;
    }

    va_list ap;
    va_start(ap, fmt);
    int nchars2 = vsnprintf(lineb + nchars, sizeof(lineb) - nchars, fmt, ap);
    va_end(ap);
    // If the required number of chars is bigger than the remaining buffer, then truncate string
    if (nchars2 > sizeof(lineb) - nchars) {
        goto full_buf;
    }

    ret = circbuf_write(&logcb, lineb, nchars + nchars2);
    log_flush();
    return ret;

full_buf:
    lineb[sizeof(lineb) - 1] = '\n';
    ret = circbuf_write(&logcb, lineb, sizeof(lineb));
    log_flush();
    return ret;
}

int log_flush(void) {
    // TODO Use board info to configure log transport (e.g. uartX)

    char buf[10] = { 0 };
    circbuf_read(&logcb, buf, sizeof(buf));


    return 0;
}
