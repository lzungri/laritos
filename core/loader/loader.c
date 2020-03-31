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

#include <libc/string.h>
#include <loader/loader.h>
#include <loader/elf.h>
#include <loader/loader-elf.h>
#include <process/types.h>
#include <process/core.h>
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
            process_set_name(pcb, f->dentry->name);
            strncpy(pcb->cmd, path, sizeof(pcb->cmd));

            vfs_file_close(f);
            return pcb;
        }
    }

    error_async("Executable format not recognized");

    vfs_file_close(f);
    return NULL;
}
