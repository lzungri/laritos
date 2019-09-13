#include <log.h>

#include <board.h>
#include <component.h>
#include <intc.h>

int intc_init(intc_t *comp, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    return component_init((component_t *) comp, id, bcomp, COMP_TYPE_GIC, init, deinit);
}
