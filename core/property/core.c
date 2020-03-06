#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <strtoxl.h>
#include <property/core.h>
#include <mm/heap.h>
#include <sync/spinlock.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <fs/core.h>
#include <utils/math.h>

int property_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.properties);
    spinlock_init(&_laritos.prop_lock);
    return 0;
}

/**
 * TODO: This is very inefficient, we should use a dictionary or rbtree
 * along with a LRU to cache the properties
 */
static inline property_t *get_property_locked(char *id) {
    property_t *p;
    list_for_each_entry(p, &_laritos.properties, list) {
        if (strncmp(p->id, id, sizeof(p->id)) == 0) {
            return p;
        }
    }
    return NULL;
}

static int sysfs_property_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    property_t *p = f->data0;
    char prop[PROPERTY_VALUE_MAX_LEN];
    if (property_get(p->id, prop) < 0) {
        error("Couldn't get property %s", p->id);
        return -1;
    }
    return pseudofs_write_to_buf(buf, blen, prop, strlen(prop), offset);
}

static int sysfs_property_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    property_t *p = f->data0;
    return property_set(p->id, (char *) buf) < 0 ? -1 : min(blen, PROPERTY_VALUE_MAX_LEN);
}

static inline int sysfs_create(property_t *p) {
    bool write_allowed = p->mode & (PROPERTY_MODE_WRITE_BY_ALL | PROPERTY_MODE_WRITE_BY_OWNER);
    fs_dentry_t *dentry;
    if (write_allowed) {
        dentry = pseudofs_create_custom_rw_file_with_dataptr(_laritos.fs.property_root, p->id,
                    sysfs_property_read, sysfs_property_write, p);
    } else {
        dentry = pseudofs_create_custom_ro_file_with_dataptr(_laritos.fs.property_root, p->id,
                    sysfs_property_read, p);
    }
    if (dentry == NULL) {
        error("Failed to create '%s' sysfs file", p->id);
        return -1;
    }
    return 0;
}

static inline int sysfs_remove(property_t *p) {
    return vfs_file_remove(_laritos.fs.property_root, p->id);
}

int property_create(char *id, char *value, prop_mode_t mode) {
    debug("Creating property %s with value='%s', mode=0x%x", id, value, mode);

    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);

    if (get_property_locked(id) != NULL) {
        error("Property %s already used", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    property_t *p = calloc(1, sizeof(property_t));
    if (p == NULL) {
        error("Couldn't create property %s", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    strncpy(p->id, id, sizeof(p->id) - 1);
    strncpy(p->value, value != NULL ? value : "", sizeof(p->value) - 1);
    INIT_LIST_HEAD(&p->list);
    p->mode = mode;

    list_add_tail(&p->list, &_laritos.properties);

    // If it is free for all, no need to save the owner
    if (p->mode & (PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER)) {
        irqctx_t pcbdata_ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);

        pcb_t *pcb = process_get_current();
        p->owner = pcb;
        ref_inc(&pcb->refcnt);

        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
    }

    sysfs_create(p);

    spinlock_release(&_laritos.prop_lock, &ctx);

    return 0;
}

int property_remove(char *id) {
    debug("Removing property %s", id);

    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);

    property_t *p = get_property_locked(id);
    if (p == NULL) {
        error("Property %s doesn't exist", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    if ((p->mode & PROPERTY_MODE_WRITE_BY_OWNER) && p->owner != process_get_current()) {
        error("Cannot remove %s, not the property owner", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    sysfs_remove(p);

    list_del_init(&p->list);

    if (p->mode & (PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER)) {
        irqctx_t pcbdata_ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);

        ref_dec(&p->owner->refcnt);
        p->owner = NULL;

        spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
    }

    spinlock_release(&_laritos.prop_lock, &ctx);

    free(p);
    return 0;
}

int property_set(char *id, char *value) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);

    property_t *p = get_property_locked(id);
    if (p == NULL) {
        error("Property %s doesn't exist", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    if ((p->mode & PROPERTY_MODE_WRITE_BY_OWNER) && p->owner != process_get_current()) {
        error("Cannot write %s, not the property owner", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }
    if (!(p->mode & (PROPERTY_MODE_WRITE_BY_ALL | PROPERTY_MODE_WRITE_BY_OWNER))) {
        error("Cannot write %s, read-only property", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    strncpy(p->value, value, sizeof(p->value) - 1);

    spinlock_release(&_laritos.prop_lock, &ctx);

    return 0;
}

int property_get(char *id, char *buf) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);

    property_t *p = get_property_locked(id);
    if (p == NULL) {
        verbose("Property %s doesn't exist", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    if ((p->mode & PROPERTY_MODE_READ_BY_OWNER) && p->owner != process_get_current()) {
        error("Cannot read %s, not the property owner", id);
        spinlock_release(&_laritos.prop_lock, &ctx);
        return -1;
    }

    strncpy(buf, p->value, sizeof(p->value) - 1);

    spinlock_release(&_laritos.prop_lock, &ctx);

    return 0;
}

void property_get_or_def(char *id, char *buf, char *def) {
    if (property_get(id, buf) < 0) {
        strncpy(buf, def, PROPERTY_VALUE_MAX_LEN - 1);
    }
}

int property_get_int32(char *id, int32_t *buf) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    if (property_get(id, prop) < 0) {
        verbose("Couldn't get property %s", id);
        return -1;
    }
    *buf = (int32_t) strtol(prop, NULL, 0);
    return 0;
}

int32_t property_get_or_def_int32(char *id, int32_t def) {
    int32_t v;
    if (property_get_int32(id, &v) < 0) {
        v = def;
    }
    return v;
}

static int create_root_sysfs(sysfs_mod_t *sysfs) {
    _laritos.fs.property_root = vfs_dir_create(_laritos.fs.kernelfs_root, "property",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (_laritos.fs.property_root == NULL) {
        error("Error creating property sysfs directory");
        return -1;
    }
    return 0;
}

static int remove_root_sysfs(sysfs_mod_t *sysfs) {
    return vfs_dir_remove(_laritos.fs.kernelfs_root, "property");
}

SYSFS_MODULE(property, create_root_sysfs, remove_root_sysfs)



#ifdef CONFIG_TEST_CORE_PROPERTY_CORE
#include __FILE__
#endif
