#include <log.h>

#include <component/ticker.h>
#include <component/timer.h>
#include <component/component.h>
#include <driver/driver.h>
#include <mm/heap.h>

static int init(component_t *c) {
    return ticker_init((ticker_comp_t *) c);
}

static int deinit(component_t *c) {
    return ticker_deinit((ticker_comp_t *) c);
}

static int process(board_comp_t *comp) {
    ticker_comp_t *t = component_alloc(sizeof(ticker_comp_t));
    if (t == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (ticker_component_init(t, comp, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }

    component_set_info((component_t *) t, "Ticker", "lzungri", "Generic OS ticker");

    if (component_register((component_t *) t) < 0) {
        error("Couldn't register ticker '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(t);
    return -1;
}

DEF_DRIVER_MANAGER(generic_ticker, process);
