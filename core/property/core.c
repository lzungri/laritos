#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <property/core.h>
#include <mm/heap.h>
#include <sync/spinlock.h>

int property_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.properties);
    spinlock_init(&_laritos.prop_lock);
    return 0;
}

static inline property_t *get_property(char *id) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);
    property_t *p;
    list_for_each_entry(p, &_laritos.properties, list) {
        if (strncmp(p->id, id, sizeof(p->id)) == 0) {
            spinlock_release(&_laritos.prop_lock, &ctx);
            return p;
        }
    }
    spinlock_release(&_laritos.prop_lock, &ctx);
    return NULL;
}

int property_create(char *id, prop_mode_t mode) {
    debug("Creating property %s with mode=0x%x", id, mode);

    if (get_property(id) != NULL) {
        error("Property %s already used", id);
        return -1;
    }

    property_t *p = calloc(1, sizeof(property_t));
    if (p == NULL) {
        error("Couldn't create property %s", id);
        return -1;
    }

    strncpy(p->id, id, sizeof(p->id) - 1);
    INIT_LIST_HEAD(&p->list);
    p->mode = mode;

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    pcb_t *pcb = process_get_current();
    p->owner = pcb;
    ref_inc(&pcb->refcnt);

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    spinlock_acquire(&_laritos.prop_lock, &ctx);
    list_add_tail(&p->list, &_laritos.properties);
    spinlock_release(&_laritos.prop_lock, &ctx);

    return 0;
}

int property_remove(char *id) {
    debug("Removing property %s", id);

    property_t *p = get_property(id);
    if (p == NULL) {
        error("Property %s doesn't exist", id);
        return -1;
    }

    irqctx_t ctx;
    spinlock_acquire(&_laritos.prop_lock, &ctx);
    list_del_init(&p->list);
    spinlock_release(&_laritos.prop_lock, &ctx);

    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    ref_dec(&p->owner->refcnt);
    p->owner = NULL;

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    free(p);
    return 0;
}

int property_set(char *id, char *value) {

    return -1;
}

int property_get(char *id, char *buf) {

    return -1;
}

int property_get_int32(char *id, int32_t *buf) {

    return -1;
}



#ifdef CONFIG_TEST_CORE_PROPERTY_CORE
#include __FILE__
#endif
