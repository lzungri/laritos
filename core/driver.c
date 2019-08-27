#include <log.h>
#include <driver.h>
#include <board.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int search_drivermgr_and_process(board_info_t *bi, board_comp_t *comp);

static int search_drivermgr_and_process_by_comp_id(board_info_t *bi, char *cid) {
    int i;
    for (i = 0; i < bi->len; i++) {
        if (strncmp(bi->components[i].id, cid, BOARD_MAX_COMP_ID_LEN_BYTES) == 0) {
            return search_drivermgr_and_process(bi, &bi->components[i]);
        }
    }
    return -1;
}

static int process_dependencies(board_info_t *bi, board_comp_t *comp) {
    char deps[CONFIG_BOARD_MAX_COMPONENT_ATTRS][BOARD_MAX_ATTR_VALUE_LEN_BYTES] = { 0 };

    int ndeps = 0;
    do {
        board_get_str_attr_idx(comp, "depends", deps[ndeps], ndeps, "");
    } while(ndeps < CONFIG_BOARD_MAX_COMPONENT_ATTRS && strnlen(deps[ndeps++], BOARD_MAX_ATTR_VALUE_LEN_BYTES) > 0);

    ndeps--;
    if (ndeps > 0) {
        int i;
        for (i = 0; i < ndeps; i++) {
            debug("'%s' depends on '%s'", comp->id, deps[i]);
            if (search_drivermgr_and_process_by_comp_id(bi, deps[i]) < 0) {
                error("Error while processing component dependency '%s'", deps[i]);
                return -1;
            }
        }
    }

    return 0;
}

static int search_drivermgr_and_process(board_info_t *bi, board_comp_t *comp) {
    if (comp->processed) {
        debug("Component '%s' already processed, skipping", comp->id);
        return 0;
    }

    debug("Searching driver manager '%s' for component '%s'", comp->driver, comp->id);

    driver_mgr_t **dptr;
    for (dptr = __driver_mgrs_start; *dptr; dptr++) {
        driver_mgr_t *d = *dptr;

        if (strncmp(comp->driver, d->name, CONFIG_DRIVER_NAME_MAX_LEN) == 0) {
            if (process_dependencies(bi, comp) < 0) {
                error("Failed to process dependencies for component '%s'", comp->id);
                return -1;
            }

            info("Processing driver '%s' for component '%s'", comp->driver, comp->id);
            if (d->process(comp) < 0) {
                error("Couldn't process driver '%s' for component '%s'", d->name, comp->id);
                return -1;
            }
            comp->processed = true;
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
        if (search_drivermgr_and_process(bi, &bi->components[i]) < 0) {
            error("Could not process component '%s'", bi->components[i].id);
            nerrors++;
        }
    }
    return -nerrors;
}

