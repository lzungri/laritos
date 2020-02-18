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
