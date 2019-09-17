#include <log.h>
#include <driver.h>
#include <inputdev.h>
#include <board-types.h>
#include <board.h>


#define MAX_INPUTDEVS 3

// TODO Use dynamic memory instead
static inputdev_t inputdevs[MAX_INPUTDEVS];
static uint8_t ninputs;

static int process(board_comp_t *comp) {
    if (ninputs > ARRAYSIZE(inputdevs)) {
        error("Max number of input components reached");
        return -1;
    }

    inputdev_t *idev = &inputdevs[ninputs];
    if (component_init((component_t *) idev, comp->id, comp, COMP_TYPE_INPUTDEV, NULL, NULL) < 0){
        error("Failed to initialize '%s'", comp->id);
        return -1;
    }

    if (board_get_component_attr(comp, "transport", (component_t **) &idev->transport) < 0 ||
            idev->transport->ops.read == NULL) {
        error("No valid transport found for inputdev '%s'", comp->id);
        return -1;
    }

    if (component_register((component_t *) idev) < 0) {
        error("Couldn't register inputdev '%s'", comp->id);
        return -1;
    }
    ninputs++;

    return 0;
}

DEF_DRIVER_MANAGER(inputdev, process);
