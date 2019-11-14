#include <log.h>

#include <board-types.h>
#include <component/component.h>
#include <string.h>
#include <core.h>
#include <utils/utils.h>
#include <dstruct/list.h>
#include <mm/heap.h>

int component_init_global_context() {
    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.comps); i++) {
        INIT_LIST_HEAD(&_laritos.comps[i]);
    }
    return 0;
}

int component_deinit_global_context() {
    // Nothing
    return 0;
}

static int nop_init(component_t *c) {
    // Nothing
    return 0;
}

static int nop_deinit(component_t *c) {
    // Nothing
    return 0;
}

void *component_alloc(size_t size) {
    component_t *c = calloc(1, size);
    if (c == NULL) {
        return NULL;
    }
    c->ops.free = (void *(*)(component_t *))free;
    return c;
}

int component_init(component_t *comp, char *id, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, id, sizeof(comp->id) - 1);
    comp->type = type;
    comp->ops.init = init == NULL ? nop_init : init;
    comp->ops.deinit = deinit == NULL ? nop_deinit : deinit;
    INIT_LIST_HEAD(&comp->list);
    return 0;
}

int component_register(component_t *comp) {
    debug("Registering component '%s' of type %d", comp->id, comp->type);
    list_add(&comp->list, &_laritos.comps[comp->type]);

    verbose("Initializing component '%s'", comp->id);
    if (comp->ops.init(comp) < 0) {
        error("Couldn't initialize component '%s'", comp->id);
        list_del(&comp->list);
        return -1;
    }
    info("'%s' component (type %d) registered", comp->id, comp->type);
    return 0;
}

int component_unregister(component_t *comp) {
    verbose("De-initializing component '%s'", comp->id);
    list_del(&comp->list);
    if (comp->ops.deinit(comp) < 0) {
        error("Couldn't de-initialize component '%s'", comp->id);
    }
    if (comp->ops.free != NULL) {
        verbose("Freeing component '%s'", comp->id);
        comp->ops.free(comp);
    }
    info("Component '%s' of type %d unregistered", comp->id, comp->type);
    return 0;
}

int component_set_info(component_t *comp, char *product, char *vendor, char *description) {
    strncpy(comp->product, product, sizeof(comp->product) - 1);
    strncpy(comp->vendor, vendor, sizeof(comp->vendor) - 1);
    strncpy(comp->description, description, sizeof(comp->description) - 1);
    return 0;
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

bool component_any_of(component_type_t t) {
    return !list_empty(&_laritos.comps[t]);
}

bool component_are_mandatory_comps_present(void) {
    component_type_t mand[] = { COMP_TYPE_CPU, COMP_TYPE_RTC };
    int i;
    for (i = 0; i < ARRAYSIZE(mand); i++) {
        if (component_any_of(mand[i])) {
            continue;
        }
        info("No type %d component was found", mand[i]);
        return false;
    }
    return true;
}
