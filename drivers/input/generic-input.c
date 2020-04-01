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
