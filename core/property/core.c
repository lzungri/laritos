#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <strtoxl.h>
#include <property/core.h>
#include <mm/heap.h>
#include <sync/spinlock.h>

int property_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.properties);
    spinlock_init(&_laritos.prop_lock);
    return 0;
}

static inline property_t *get_property_locked(char *id) {
    property_t *p;
    list_for_each_entry(p, &_laritos.properties, list) {
        if (strncmp(p->id, id, sizeof(p->id)) == 0) {
            return p;
        }
    }
    return NULL;
}

int property_create(char *id, prop_mode_t mode) {
    debug("Creating property %s with mode=0x%x", id, mode);

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

    strncpy(p->value, value, sizeof(p->value) - 1);

    spinlock_release(&_laritos.prop_lock, &ctx);

    return 0;
}

int property_get(char *id, char *buf) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);

    property_t *p = get_property_locked(id);
    if (p == NULL) {
        error("Property %s doesn't exist", id);
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

int property_get_int32(char *id, int32_t *buf) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    if (property_get(id, prop) < 0) {
        error("Couldn't get property %s", id);
        return -1;
    }
    *buf = (int32_t) strtol(prop, NULL, 0);
    return 0;
}



#ifdef CONFIG_TEST_CORE_PROPERTY_CORE
#include __FILE__
#endif
