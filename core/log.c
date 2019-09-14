#include <log.h>
#include <stdarg.h>
#include <printf.h>
#include <stdbool.h>

#include <board.h>
#include <driver.h>
#include <circbuf.h>
#include <utils.h>
#include <component.h>
#include <stream.h>


static char logb[CONFIG_LOG_BUFSIZE_BYTES];

DEF_STATIC_CIRCBUF(logcb, logb, sizeof(logb));


int __add_log_msg(bool sync, char *level, char *tag, char *fmt, ...) {
    int ret = 0;
    char lineb[CONFIG_LOG_MAX_LINE_SIZE] = { 0 };

    int nchars = snprintf(lineb, sizeof(lineb), "[XXX.xxx] %s %s: ", level, tag);
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
    if (sync) {
        log_flush();
    }
    return ret;

full_buf:
    lineb[sizeof(lineb) - 1] = '\n';
    ret = circbuf_write(&logcb, lineb, sizeof(lineb));
    if (sync) {
        log_flush();
    }
    return ret;
}


typedef struct {
    component_t parent;
    stream_t *transport;
} logger_t;

// TODO Use dynamic memory instead
static logger_t loggers[3];
static uint8_t nloggers;

int log_flush(void) {
    if (nloggers == 0) {
        return 0;
    }

    int bread = 0;
    char buf[CONFIG_LOG_MAX_LINE_SIZE * 5] = { 0 };
    while ((bread = circbuf_read(&logcb, buf, sizeof(buf))) > 0) {
        int i;
        for (i = 0; i < nloggers; i++) {
            stream_t *s = loggers[i].transport;
            s->ops.write(s, buf, bread);
        }
    }
    return 0;
}

static int process(board_comp_t *comp) {
    if (nloggers > ARRAYSIZE(loggers)) {
        error("Max number of loggers reached");
        return -1;
    }

    logger_t *logger = &loggers[nloggers];

    if (component_init((component_t *) logger, comp->id, comp, COMP_TYPE_LOGGER, NULL, NULL) < 0) {
        error("Failed to initialize logger '%s'", comp->id);
        return -1;
    }

    char tp[CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };
    board_get_str_attr_def(comp, "transport", tp, "");
    logger->transport = (stream_t *) component_get_by_id(tp);
    if (logger->transport == NULL || logger->transport->ops.write == NULL) {
        error("No valid transport found for logger '%s'", comp->id);
        return -1;
    }

    if (component_register((component_t *) logger) < 0) {
        error("Couldn't register logger '%s'", comp->id);
        return -1;
    }

    nloggers++;

    // Now that we have a logger ready, flush all the previously buffered data
    log_flush();

    return 0;
}

DEF_DRIVER_MANAGER(logger, process);
