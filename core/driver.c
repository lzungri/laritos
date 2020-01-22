#include <log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <core.h>
#include <board/board-types.h>
#include <board/board.h>
#include <driver/driver.h>

static int search_drivermgr_and_process(board_info_t *bi, board_comp_t *comp);

static int search_drivermgr_and_process_by_comp_id(board_info_t *bi, char *cid) {
    int i;
    for (i = 0; i < bi->len; i++) {
        if (strncmp(bi->components[i].id, cid, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES) == 0) {
            return search_drivermgr_and_process(bi, &bi->components[i]);
        }
    }
    return -1;
}

static int process_dependencies(board_info_t *bi, board_comp_t *comp) {
    char deps[CONFIG_BOARD_MAX_COMPONENT_ATTRS][CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES] = { 0 };

    int ndeps = 0;
    board_comp_attr_t *attr;
    for_each_bc_attr(comp, attr) {
        char *pos;
        if ((pos = strrchr(attr->value, '@')) != NULL) {
            strncpy(deps[ndeps++], pos + 1, sizeof(deps[0]));
        }
    }

    if (ndeps > 0) {
        int i;
        for (i = 0; i < ndeps; i++) {
            verbose("'%s' depends on '%s'", comp->id, deps[i]);
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

        if (strncmp(comp->driver, d->name, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES) == 0) {
            if (process_dependencies(bi, comp) < 0) {
                error("Failed to process dependencies for component '%s'", comp->id);
                return -1;
            }

            verbose("Processing driver '%s' for component '%s'", comp->driver, comp->id);
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

    if (nerrors > 0) {
        return -nerrors;
    }

    if (!component_are_mandatory_comps_present()) {
        error("Not all mandatory board components were found");
        return -1;
    }

    _laritos.components_loaded = true;
    return 0;
}
