#include <log.h>
#include <driver.h>
#include <board.h>
#include <intc.h>
#include <drivers/gicv2.h>

#define MAX_GICS 1
// TODO Use dynamic memory instead
static gic_t gics[MAX_GICS];
static uint8_t cur_gic;

static int process(board_comp_t *comp) {
    if (cur_gic > ARRAYSIZE(gics)) {
        error("Max number of GICs components reached");
        return -1;
    }

    gic_t *gic = &gics[cur_gic];

    if (intc_init((intc_t *) gic, comp->id, comp, NULL, NULL) < 0) {
        error("Failed to initialize gic '%s'", comp->id);
        return -1;
    }

    board_get_ptr_attr(comp, "distaddr", (void **) &gic->dist, NULL);
    if (gic->dist == NULL) {
        error("No distributor address was specified in the board information for '%s'", comp->id);
        return -1;
    }

    board_get_ptr_attr(comp, "cpuaddr", (void **) &gic->cpu, NULL);
    if (gic->cpu == NULL) {
        error("No cpu address was specified in the board information for '%s'", comp->id);
        return -1;
    }

    if (component_register((component_t *) gic) < 0) {
        error("Couldn't register gic '%s'", comp->id);
        return -1;
    }
    cur_gic++;

    return 0;
}

DEF_DRIVER_MANAGER(gicv2, process);
