#include <log.h>

#include <board/core.h>
#include <board/types.h>
#include <component/component.h>
#include <string.h>
#include <core.h>
#include <utils/utils.h>
#include <dstruct/list.h>
#include <mm/heap.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>

int component_init_global_context() {
    int i;
    for (i = 0; i < ARRAYSIZE(_laritos.comps); i++) {
        INIT_LIST_HEAD(&_laritos.comps[i]);
    }
    return 0;
}

static int nop_init(component_t *c) {
    // Nothing
    return 0;
}

static int nop_deinit(component_t *c) {
    // Nothing
    return 0;
}

void *component_alloc(size_t size) {
    component_t *c = calloc(1, size);
    if (c == NULL) {
        return NULL;
    }
    c->ops.free = (void *(*)(component_t *))free;
    return c;
}

int component_init(component_t *comp, char *id, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {
    strncpy(comp->id, id, sizeof(comp->id) - 1);
    comp->type = type;
    comp->ops.init = init == NULL ? nop_init : init;
    comp->ops.deinit = deinit == NULL ? nop_deinit : deinit;
    INIT_LIST_HEAD(&comp->list);

    board_get_bool_attr_def(bcomp, "default", &comp->dflt, false);
    return 0;
}

static int product_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    component_t *c = f->data0;
    return pseudofs_write_to_buf(buf, blen, c->product, sizeof(c->product), offset);
}

static int vendor_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    component_t *c = f->data0;
    return pseudofs_write_to_buf(buf, blen, c->vendor, sizeof(c->vendor), offset);
}

static int desc_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    component_t *c = f->data0;
    return pseudofs_write_to_buf(buf, blen, c->description, sizeof(c->description), offset);
}

static int default_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    component_t *c = f->data0;
    return pseudofs_write_to_buf(buf, blen, c->dflt ? "1" : "0", 2, offset);
}

static int create_component_sysfs(component_t *c) {
    fs_dentry_t *compdir = vfs_dir_create(_laritos.fs.comp_info_root, c->id,
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (compdir == NULL) {
        error("Error creating '%s' sysfs directory", c->id);
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(compdir, "product", product_read, c) == NULL) {
        error("Failed to create 'freq' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(compdir, "vendor", vendor_read, c) == NULL) {
        error("Failed to create 'vendor' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(compdir, "desc", desc_read, c) == NULL) {
        error("Failed to create 'desc' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(compdir, "default", default_read, c) == NULL) {
        error("Failed to create 'default' sysfs file");
        return -1;
    }
    return 0;
}

static int remove_component_sysfs(component_t *c) {
    return vfs_dir_remove(_laritos.fs.comp_info_root, c->id);
}

int component_register(component_t *comp) {
    debug("Registering component '%s' of type %d", comp->id, comp->type);

    // If it is the default component, then add it as the first element in the list
    if (comp->dflt) {
        list_add(&comp->list, &_laritos.comps[comp->type]);
    } else {
        list_add_tail(&comp->list, &_laritos.comps[comp->type]);
    }

    verbose("Initializing component '%s'", comp->id);
    if (comp->ops.init(comp) < 0) {
        error("Couldn't initialize component '%s'", comp->id);
        list_del(&comp->list);
        return -1;
    }

    create_component_sysfs(comp);

    info("'%s' %s component registered%s", comp->id,
            component_get_type_str(comp->type), comp->dflt ? " (default)" : "");
    return 0;
}

int component_unregister(component_t *comp) {
    verbose("De-initializing component '%s'", comp->id);

    remove_component_sysfs(comp);

    list_del(&comp->list);
    if (comp->ops.deinit(comp) < 0) {
        error("Couldn't de-initialize component '%s'", comp->id);
    }

    if (comp->ops.free != NULL) {
        verbose("Freeing component '%s'", comp->id);
        comp->ops.free(comp);
    }

    info("Component '%s' of type %d unregistered", comp->id, comp->type);
    return 0;
}

int component_set_info(component_t *comp, char *product, char *vendor, char *description) {
    strncpy(comp->product, product, sizeof(comp->product) - 1);
    strncpy(comp->vendor, vendor, sizeof(comp->vendor) - 1);
    strncpy(comp->description, description, sizeof(comp->description) - 1);
    return 0;
}

component_t *component_get_by_id(char *id) {
    component_t *c;
    for_each_component(c) {
        if (strncmp(c->id, id, COMPONENT_MAX_ID_LEN) == 0) {
            return c;
        }
    }
    return NULL;
}

bool component_any_of(component_type_t t) {
    return !list_empty(&_laritos.comps[t]);
}

bool component_are_mandatory_comps_present(void) {
    component_type_t mand[] = { COMP_TYPE_CPU, COMP_TYPE_RTC, COMP_TYPE_VRTIMER,
                                COMP_TYPE_SCHED, COMP_TYPE_TICKER };
    int i;
    for (i = 0; i < ARRAYSIZE(mand); i++) {
        if (component_any_of(mand[i])) {
            continue;
        }
        info("No type %d component was found", mand[i]);
        return false;
    }
    return true;
}

static int create_root_sysfs(sysfs_mod_t *sysfs) {
    _laritos.fs.comp_root = vfs_dir_create(_laritos.fs.sysfs_root, "component",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.comp_root == NULL) {
        error("Error creating component sysfs directory");
        return -1;
    }

    _laritos.fs.comp_info_root = vfs_dir_create(_laritos.fs.comp_root, "info",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.comp_info_root == NULL) {
        error("Error creating component/info sysfs directory");
        return -1;
    }

    _laritos.fs.comp_type_root = vfs_dir_create(_laritos.fs.comp_root, "type",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.comp_type_root == NULL) {
        error("Error creating component/type sysfs directory");
        return -1;
    }

    return 0;
}

static int remove_root_sysfs(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.sysfs_root, "component");
}


SYSFS_MODULE(component, create_root_sysfs, remove_root_sysfs)
