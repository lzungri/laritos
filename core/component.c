#include <log.h>

#include <board.h>
#include <component.h>
#include <string.h>

typedef int (*comp_func)(struct component_t *);

int component_init(component_t *comp, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, bcomp->id, sizeof(comp->id));
    comp->type = type;
    comp->ops.init = (comp_func) init;
    comp->ops.deinit = (comp_func) deinit;
    return 0;
}

int component_register(component_t *comp) {
    return 0;
}
