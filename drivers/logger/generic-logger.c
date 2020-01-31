#include <log.h>

#include <stdbool.h>
#include <component/component.h>
#include <component/logger.h>
#include <component/stream.h>
#include <mm/heap.h>

static int log_write(logger_comp_t *l, const void *buf, size_t n, bool blocking) {
    return l->transport->ops.write(l->transport, buf, n, blocking);
}

static int init(component_t *c) {
    // Now that we have a logger ready, flush all the previously buffered data
    log_flush();
    return 0;
}

static int process(board_comp_t *comp) {
    logger_comp_t *logger = component_alloc(sizeof(logger_comp_t));
    if (logger == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (component_init((component_t *) logger, comp->id, comp, COMP_TYPE_LOGGER, init, NULL) < 0) {
        error("Failed to initialize logger '%s'", comp->id);
        goto fail;
    }

    logger->ops.write = log_write;

    if (board_get_component_attr(comp, "transport", (component_t **) &logger->transport) < 0 ||
            logger->transport->ops.write == NULL) {
        error("No valid transport found for logger '%s'", comp->id);
        goto fail;
    }

    if (component_register((component_t *) logger) < 0) {
        error("Couldn't register logger '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(logger);
    return -1;
}

DRIVER_MODULE(generic_logger, process);
