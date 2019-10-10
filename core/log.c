#include <log.h>
#include <stdarg.h>
#include <printf.h>
#include <stdbool.h>
#include <string.h>

#include <board-types.h>
#include <board.h>
#include <driver/driver.h>
#include <dstruct/circbuf.h>
#include <utils/utils.h>
#include <time/time.h>
#include <mm/heap.h>
#include <dstruct/list.h>
#include <component/timer.h>
#include <component/component.h>
#include <component/stream.h>


static char logb[CONFIG_LOG_BUFSIZE_BYTES];

DEF_STATIC_CIRCBUF(logcb, logb, sizeof(logb));


int __add_log_msg(bool sync, char *level, char *tag, char *fmt, ...) {
    int ret = 0;
    char lineb[CONFIG_LOG_MAX_LINE_SIZE] = { 0 };

    calendar_t cal = { 0 };
    if (rtc_get_localtime_calendar(&cal) < 0) {
        memset(&cal, 0, sizeof(cal));
    }

    // TODO: Add msecs resolution
    int nchars = snprintf(lineb, sizeof(lineb), "[%02d/%02d %02d:%02d:%02d] %s %s: ",
            cal.mon + 1, cal.mday, cal.hour, cal.min, cal.sec, level, tag);
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

    ret = circbuf_nb_write(&logcb, lineb, nchars + nchars2);
    if (sync) {
        log_flush();
    }
    return ret;

full_buf:
    lineb[sizeof(lineb) - 1] = '\n';
    ret = circbuf_nb_write(&logcb, lineb, sizeof(lineb));
    if (sync) {
        log_flush();
    }
    return ret;
}


typedef struct {
    component_t parent;
    stream_t *transport;
} logger_t;

int log_flush(void) {
    if (!component_any_of(COMP_TYPE_LOGGER)) {
        return 0;
    }

    int bread = 0;
    char buf[CONFIG_LOG_MAX_LINE_SIZE * 5] = { 0 };
    while ((bread = circbuf_nb_read(&logcb, buf, sizeof(buf))) > 0) {
        component_t *c;
        for_each_component_type(c, COMP_TYPE_LOGGER) {
            stream_t *s = ((logger_t *) c)->transport;
            s->ops.write(s, buf, bread, false);
        }
    }
    return 0;
}

static int process(board_comp_t *comp) {
    logger_t *logger = component_alloc(sizeof(logger_t));
    if (logger == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (component_init((component_t *) logger, comp->id, comp, COMP_TYPE_LOGGER, NULL, NULL) < 0) {
        error("Failed to initialize logger '%s'", comp->id);
        goto fail;
    }

    if (board_get_component_attr(comp, "transport", (component_t **) &logger->transport) < 0 ||
            logger->transport->ops.write == NULL) {
        error("No valid transport found for logger '%s'", comp->id);
        goto fail;
    }

    if (component_register((component_t *) logger) < 0) {
        error("Couldn't register logger '%s'", comp->id);
        goto fail;
    }

    // Now that we have a logger ready, flush all the previously buffered data
    log_flush();

    return 0;

fail:
    free(logger);
    return -1;
}

DEF_DRIVER_MANAGER(logger, process);
