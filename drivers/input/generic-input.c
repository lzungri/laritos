#include <log.h>
#include <board/types.h>
#include <board/core.h>
#include <mm/heap.h>
#include <component/inputdev.h>
#include <driver/core.h>


static int process(board_comp_t *comp) {
    inputdev_t *idev = component_alloc(sizeof(inputdev_t));
    if (idev == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (inputdev_component_init(idev, comp->id, comp, NULL, NULL) < 0){
        error("Failed to initialize '%s'", comp->id);
        goto fail;
    }

    if (inputdev_component_register(idev) < 0) {
        error("Couldn't register inputdev '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(idev);
    return -1;
}

DRIVER_MODULE(gen_input, process);
