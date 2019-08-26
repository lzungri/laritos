#include <log.h>

#include <board.h>
#include <component.h>
#include <string.h>

int component_init(component_t *comp, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, bcomp->id, sizeof(comp->id) - 1);
    comp->type = type;
    comp->ops.init = init;
    comp->ops.deinit = deinit;
    return 0;
}

int component_register(component_t *comp) {
    if (comp->ops.init(comp) < 0) {
        error("Couldn't initialize component '%s'", comp->id);
        return -1;
    }
    return 0;
}
