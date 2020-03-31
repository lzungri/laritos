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
