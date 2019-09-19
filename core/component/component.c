#include <log.h>

#include <board-types.h>
#include <component/component.h>
#include <string.h>
#include <core.h>
#include <utils.h>

static int nop_init(component_t *c) {
    // Nothing
    return 0;
}

static int nop_deinit(component_t *c) {
    // Nothing
    return 0;
}

int component_init(component_t *comp, char *id, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, id, sizeof(comp->id) - 1);
    comp->type = type;
    comp->ops.init = init == NULL ? nop_init : init;
    comp->ops.deinit = deinit == NULL ? nop_deinit : deinit;
    return 0;
}

int component_register(component_t *comp) {
    debug("Registering component '%s' of type %d", comp->id, comp->type);

    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.components); i++) {
        if (_laritos.components[i] == NULL) {
            _laritos.components[i] = comp;
            break;
        }
    }

    if (i >= ARRAYSIZE(_laritos.components)) {
        error("Failed to register component '%s'. Max number of components reached", comp->id);
        return -1;
    }

    verbose("Initializing component '%s'", comp->id);
    if (comp->ops.init(comp) < 0) {
        error("Couldn't initialize component '%s'", comp->id);
        _laritos.components[i] = NULL;
        return -1;
    }
    info("Component '%s' of type %d registered successfully", comp->id, comp->type);
    return 0;
}

int component_unregister(component_t *comp) {
    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.components); i++) {
        if (_laritos.components[i] == comp) {
            verbose("De-initializing component '%s'", comp->id);
            if (comp->ops.deinit(comp) < 0) {
                error("Couldn't de-initialize component '%s'", comp->id);
            }
            _laritos.components[i] = NULL;
            info("Component '%s' of type %d unregistered", comp->id, comp->type);
            return 0;
        }
    }
    error("Couldn't find and unregister component '%s'", comp->id);
    return -1;
}

component_t *component_get_by_id(char *id) {
    component_t *c;
    for_each_component(c) {
        if (strncmp(c->id, id, COMPONENT_MAX_ID_LEN) == 0) {
            return c;
        }
    }
    return NULL;
}

void component_dump_registered_comps(void) {
    component_t *c;
    debug("Components:");
    for_each_component(c) {
        debug("\t%s, type: %d", c->id, c->type);
    }
}
