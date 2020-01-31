#include <log.h>
#include <board/board-types.h>
#include <board/board.h>
#include <mm/heap.h>
#include <component/inputdev.h>
#include <driver/core.h>


static int process(board_comp_t *comp) {
    inputdev_t *idev = component_alloc(sizeof(inputdev_t));
    if (idev == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (component_init((component_t *) idev, comp->id, comp, COMP_TYPE_INPUTDEV, NULL, NULL) < 0){
        error("Failed to initialize '%s'", comp->id);
        goto fail;
    }

    if (board_get_component_attr(comp, "transport", (component_t **) &idev->transport) < 0 ||
            idev->transport->ops.read == NULL) {
        error("No valid transport found for inputdev '%s'", comp->id);
        goto fail;
    }

    if (component_register((component_t *) idev) < 0) {
        error("Couldn't register inputdev '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(idev);
    return -1;
}

DRIVER_MODULE(inputdev, process);
