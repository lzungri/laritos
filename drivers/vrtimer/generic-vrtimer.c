/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
