#include <log.h>

#include <libc/string.h>
#include <loader/loader.h>
#include <loader/elf.h>
#include <loader/loader-elf.h>
#include <process/types.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>


int loader_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.loaders);
    return 0;
}

static loader_type_t *get_loader(char *type) {
    loader_type_t *lt;
    list_for_each_entry(lt, &_laritos.loaders, list) {
        if (strncmp(lt->id, type, strlen(lt->id)) == 0) {
            return lt;
        }
    }
    return NULL;
}

int loader_register_loader_type(loader_type_t *loader) {
    if (get_loader(loader->id) != NULL) {
        error("%s loader type already registered", loader->id);
        return -1;
    }

    if (loader->id == NULL || loader->id[0] == '\0') {
        error("Loader type cannot have a null or empty id");
        return -1;
    }

    if (loader->can_handle == NULL || loader->load == NULL) {
        error("One or more mandatory loader functions are missing");
        return -1;
    }

    debug("Registering loader type '%s'", loader->id);
    list_add_tail(&loader->list, &_laritos.loaders);
    return 0;
}

int loader_unregister_loader_type(loader_type_t *loader) {
    if (loader == NULL) {
        return -1;
    }

    if (get_loader(loader->id) == NULL) {
        error("%s loader type not registered", loader->id);
        return -1;
    }

    debug("Un-registering loader type '%s'", loader->id);
    list_del_init(&loader->list);
    return -1;
}

pcb_t *loader_load_executable_from_memory(uint16_t appidx) {
    /**
     * Offset in laritos.bin where the apps are loaded.
     * Apps are (for now) appended to laritos.bin via the tools/apps/install.sh script
     * This is temporary until we implement a better mechanism for flashing apps,
     * such as a file system on sd card.
     */
    extern char __apps_start[];
    unsigned char *executable = __apps_start;

    loader_type_t *loader;
    list_for_each_entry(loader, &_laritos.loaders, list) {
        if (loader->can_handle(executable)) {
            debug_async("Loading app at 0x%p with '%s' loader", executable, loader->id);
            pcb_t *pcb = loader->load(executable);
            if (pcb == NULL) {
                error_async("Failed to load application");
            }
            return pcb;
        }
    }
    error_async("Executable format not recognized");
    return NULL;
}
