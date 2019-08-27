#include <log.h>

#include <board.h>
#include <component.h>
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

int component_init(component_t *comp, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, bcomp->id, sizeof(comp->id) - 1);
    comp->type = type;
    comp->ops.init = init == NULL ? nop_init : init;
    comp->ops.deinit = deinit == NULL ? nop_deinit : deinit;
    return 0;
}

int component_register(component_t *comp) {
    debug("Registering component '%s' of type %d", comp->id, comp->type);

    if (comp->ops.init(comp) < 0) {
        error("Couldn't initialize component '%s'", comp->id);
        return -1;
    }

    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.components); i++) {
        if (_laritos.components[i] == NULL) {
            _laritos.components[i] = comp;
            info("Component '%s' of type %d registered successfully", comp->id, comp->type);
            return 0;
        }
    }

    error("Failed to register component '%s'. Max number of components reached", comp->id);
    if (comp->ops.deinit(comp) < 0) {
        error("Couldn't de-initialize component '%s'", comp->id);
    }
    return -1;
}

int component_unregister(component_t *comp) {
    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.components); i++) {
        if (_laritos.components[i] == comp) {
            _laritos.components[i] = NULL;
            info("Component '%s' of type %d unregistered", comp->id, comp->type);
            return 0;
        }
    }
    error("Couldn't find and unregister component '%s'", comp->id);
    return -1;
}

component_t *component_get_by_id(char *id) {
    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.components); i++) {
        if (strncmp(_laritos.components[i]->id, id, MAX_COMPONENT_ID_LEN) == 0) {
            return _laritos.components[i];
        }
    }
    return NULL;
}
