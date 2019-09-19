#include <log.h>

#include <string.h>
#include <hwcomp.h>

int hwcomp_init(hwcomp_t *comp, char *id, board_comp_t *bcomp, component_subtype_t stype,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    comp->parent.stype = stype;
    return component_init((component_t *) comp, id, bcomp, COMP_TYPE_HW, init, deinit);
}

int hwcomp_set_info(hwcomp_t *comp, char *product, char *vendor, char *description) {
    strncpy(comp->product, product, sizeof(comp->product) - 1);
    strncpy(comp->vendor, vendor, sizeof(comp->vendor) - 1);
    strncpy(comp->description, description, sizeof(comp->description) - 1);
    return 0;
}
