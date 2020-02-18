#include <log.h>

#include <component/component.h>
#include <component/ticker.h>
#include <component/vrtimer.h>
#include <mm/heap.h>

static int init(component_t *c) {
    return vrtimer_init((vrtimer_comp_t *) c);
}

static int deinit(component_t *c) {
    return vrtimer_deinit((vrtimer_comp_t *) c);
}

static int process(board_comp_t *comp) {
    vrtimer_comp_t *t = component_alloc(sizeof(vrtimer_comp_t));
    if (t == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (vrtimer_component_init(t, comp, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }

    component_set_info((component_t *) t, "VR timer", "lzungri", "Generic virtual timer");

    if (component_register((component_t *) t) < 0) {
        error("Couldn't register vrtimer '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(t);
    return -1;
}

DRIVER_MODULE(generic_vrtimer, process);
