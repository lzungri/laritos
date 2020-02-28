#include <log.h>

#include <board/types.h>
#include <board/core.h>
#include <cpu/core.h>
#include <dstruct/list.h>
#include <component/component.h>
#include <component/inputdev.h>
#include <utils/utils.h>
#include <utils/function.h>
#include <mm/heap.h>
#include <sync/atomic.h>
#include <generated/autoconf.h>


int inputdev_component_init(inputdev_t *input, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) input, bcomp->id, bcomp, COMP_TYPE_INPUTDEV, NULL, NULL) < 0){
        error("Failed to initialize '%s'", bcomp->id);
        return -1;
    }

    if (board_get_component_attr(bcomp, "transport", (component_t **) &input->transport) < 0 ||
            input->transport->ops.read == NULL) {
        error("No valid transport found for inputdev '%s'", bcomp->id);
        return -1;
    }

    return 0;
}

int inputdev_component_register(inputdev_t *input) {
    if (component_register((component_t *) input) < 0) {
        error("Couldn't register '%s'", input->parent.id);
        return -1;
    }
    return 0;
}
