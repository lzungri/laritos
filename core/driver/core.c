/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <core.h>
#include <dstruct/list.h>
#include <board/types.h>
#include <board/core.h>
#include <driver/core.h>


int driver_init_global_context() {
    INIT_LIST_HEAD(&_laritos.drivers);
    return 0;
}

int driver_register(driver_t *d, module_t *owner) {
    debug("Registering driver '%s'", d->id);
    list_add_tail(&d->list, &_laritos.drivers);
    return 0;
}

int driver_unregister(driver_t *d, module_t *owner) {
    debug("Un-registering driver '%s'", d->id);
    list_del_init(&d->list);
    return 0;
}

static int search_driver_and_process(board_info_t *bi, board_comp_t *comp);

static int search_driver_and_process_by_comp_id(board_info_t *bi, char *cid) {
    int i;
    for (i = 0; i < bi->len; i++) {
        if (strncmp(bi->components[i].id, cid, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES) == 0) {
            return search_driver_and_process(bi, &bi->components[i]);
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
            if (search_driver_and_process_by_comp_id(bi, deps[i]) < 0) {
                error("Error while processing component dependency '%s'", deps[i]);
                return -1;
            }
        }
    }

    return 0;
}

static int search_driver_and_process(board_info_t *bi, board_comp_t *comp) {
    if (comp->processed) {
        debug("Component '%s' already processed, skipping", comp->id);
        return 0;
    }

    debug("Searching driver '%s' for component '%s'", comp->driver, comp->id);

    driver_t *d;
    list_for_each_entry(d, &_laritos.drivers, list) {
        if (strncmp(comp->driver, d->id, CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES) == 0) {
            if (process_dependencies(bi, comp) < 0) {
                error("Failed to process dependencies for component '%s'", comp->id);
                return -1;
            }

            verbose("Processing driver '%s' for component '%s'", comp->driver, comp->id);
            if (d->process(comp) < 0) {
                error("Couldn't process driver '%s' for component '%s'", d->id, comp->id);
                return -1;
            }
            comp->processed = true;
            return 0;
        }
    }

    error("Couldn't find driver '%s' for component '%s'", comp->driver, comp->id);
    return -1;
}

int driver_process_board_components(board_info_t *bi) {
    info("Processing board components");

    uint8_t nerrors = 0;
    int i;
    for (i = 0; i < bi->len; i++) {
        if (search_driver_and_process(bi, &bi->components[i]) < 0) {
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
