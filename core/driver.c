#include <log.h>
#include <driver.h>
#include <board.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int search_and_init_driver(board_comp_t *comp) {
    debug("Searching driver '%s' for component '%s'", comp->driver, comp->name);

    driver_t **dptr;
    for (dptr = __drivers_start; *dptr; dptr++) {
        driver_t *d = *dptr;
        if (strncmp(comp->driver, d->name, DRIVER_NAME_MAX_LEN) == 0) {
            debug("Found driver '%s' for component '%s'", comp->driver, comp->name);
            info("Initializing driver %s", d->name);
            if (d->init(comp) < 0) {
                error("Couldn't initialize driver %s", d->name);
                return -1;
            }
            d->state = DRIVER_STATE_INITIALIZED;
            debug("Driver %s initialized", d->name);

            return 0;
        }
    }

    error("Couldn't find driver '%s' for component '%s'", comp->driver, comp->name);
    return -1;
}

int driver_deinit_all(void) {
    uint8_t nerrors = 0;
    driver_t **dptr;
    for (dptr = __drivers_start; *dptr; dptr++) {
        driver_t *d = *dptr;
        if (d->state == DRIVER_STATE_INITIALIZED) {
            info("De-initializing driver %s", d->name);
            if (d->deinit() < 0) {
                error("Couldn't de-initialize driver %s", d->name);
                nerrors++;
            }
            d->state = DRIVER_STATE_NOT_INITIALIZED;
            debug("Driver %s de-initialized", d->name);
        }
    }
    return -nerrors;
}

int driver_initialize_all(board_info_t *bi) {
    info("Initializing drivers for board");

    uint8_t nerrors = 0;
    int i;
    for (i = 0; i < bi->len; i++) {
        if (search_and_init_driver(&bi->components[i]) < 0) {
            error("Could not initialize driver with name %s", bi->components[i].name);
            nerrors++;
        }
    }

    if (nerrors) {
        if (driver_deinit_all() < 0) {
            error("Error de-initializing drivers");
        }
        return -nerrors;
    }
    return 0;
}

