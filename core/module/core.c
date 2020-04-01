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

#include <core.h>
#include <dstruct/list.h>
#include <module/types.h>
#include <module/core.h>
#include <mm/heap.h>

int module_init_global_context() {
    INIT_LIST_HEAD(&_laritos.modules);
    return 0;
}

int module_load_static_modules(void) {
    // Null-terminated array of pointers to modules
    extern module_t *__modules_start[];

    module_t **mptrptr;
    for (mptrptr = __modules_start; *mptrptr; mptrptr++) {
        module_t *m = *mptrptr;

        debug("Loading module '%s'", m->id);

        if (m->init(m) < 0) {
            error("Failed to initialize module '%s'", m->id);
            return -1;
        }

        list_add_tail(&m->list, &_laritos.modules);
    }

    return 0;
}
