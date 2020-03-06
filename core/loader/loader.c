#include <log.h>

#include <libc/string.h>
#include <loader/loader.h>
#include <loader/elf.h>
#include <loader/loader-elf.h>
#include <process/types.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>


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

pcb_t *loader_load_executable_from_file(char *path) {
    fs_file_t *f = vfs_file_open(path, FS_ACCESS_MODE_READ);
    if (f == NULL) {
        error_async("Couldn't open '%s' for execution", path);
        return NULL;
    }

    if (!(f->dentry->inode->mode & FS_ACCESS_MODE_EXEC)) {
        error_async("'%s' doesn't have execution permissions", path);
        vfs_file_close(f);
        return NULL;
    }

    loader_type_t *loader;
    list_for_each_entry(loader, &_laritos.loaders, list) {
        if (loader->can_handle(f)) {
            debug_async("Loading app from '%s' with '%s' loader", path, loader->id);
            pcb_t *pcb = loader->load(f);
            if (pcb == NULL) {
                error_async("Failed to load application");
            }

            vfs_file_close(f);
            return pcb;
        }
    }

    error_async("Executable format not recognized");

    vfs_file_close(f);
    return NULL;
}
