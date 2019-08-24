#include <log.h>
#include <driver.h>
#include <board.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int search_drivermgr_and_process(board_comp_t *comp) {
    debug("Searching driver manager '%s' for component '%s'", comp->driver, comp->id);

    driver_mgr_t **dptr;
    for (dptr = __driver_mgrs_start; *dptr; dptr++) {
        driver_mgr_t *d = *dptr;
        if (strncmp(comp->driver, d->name, CONFIG_DRIVER_NAME_MAX_LEN) == 0) {
            info("Processing driver '%s' for component '%s'", comp->driver, comp->id);
            if (d->process(comp) < 0) {
                error("Couldn't process driver '%s' for component '%s'", d->name, comp->id);
                return -1;
            }
            debug("Driver '%s' processed", d->name);

            return 0;
        }
    }

    error("Couldn't find driver manager '%s' for component '%s'", comp->driver, comp->id);
    return -1;
}

int driver_process_board_components(board_info_t *bi) {
    info("Processing board components");

    uint8_t nerrors = 0;
    int i;
    for (i = 0; i < bi->len; i++) {
        if (search_drivermgr_and_process(&bi->components[i]) < 0) {
            error("Could not process component '%s'", bi->components[i].id);
            nerrors++;
        }
    }
    return -nerrors;
}

