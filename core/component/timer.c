#include <log.h>

#include <stdint.h>
#include <printf.h>
#include <board/core.h>
#include <irq/types.h>
#include <component/component.h>
#include <component/timer.h>
#include <utils/function.h>
#include <sync/spinlock.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>

int timer_init(timer_comp_t *t) {
    spinlock_init(&t->lock);
    // Setup irq stuff if using interrupt-driven io
    if (t->intio) {
        if (intc_enable_irq_with_handler(t->intc,
                t->irq, t->irq_trigger, t->irq_handler, t) < 0) {
            error("Failed to enable irq %u with handler 0x%p", t->irq, t->irq_handler);
            return -1;
        }
    }
    return 0;
}

int timer_deinit(timer_comp_t *t) {
    if (t->intio) {
        return intc_disable_irq_with_handler(t->intc, t->irq, t->irq_handler);
    }
    return 0;
}

irqret_t timer_handle_expiration(timer_comp_t *t) {
    if (!t->curtimer.enabled) {
        return IRQ_RET_HANDLED;
    }

    irqret_t ret = IRQ_RET_HANDLED;
    if (t->curtimer.cb(t, t->curtimer.data) < 0) {
        ret = IRQ_RET_ERROR;
    }

    if (t->curtimer.periodic) {
        t->ops.set_expiration_ticks(t, t->curtimer.ticks, TIMER_EXP_RELATIVE,
                t->curtimer.cb, t->curtimer.data, t->curtimer.periodic);
    }

    return ret;
}


DEF_NOT_IMPL_FUNC(ni_get_value, timer_comp_t *t, uint64_t *v);
DEF_NOT_IMPL_FUNC(ni_set_value, timer_comp_t *t, uint64_t v);
DEF_NOT_IMPL_FUNC(ni_get_remaining, timer_comp_t *t, int64_t *v);
DEF_NOT_IMPL_FUNC(ni_reset, timer_comp_t *t);
DEF_NOT_IMPL_FUNC(ni_set_enable, timer_comp_t *t, bool enable);
DEF_NOT_IMPL_FUNC(ni_set_expiration_ticks, timer_comp_t *t, int64_t timer_ticks, timer_exp_type_t type,
        timer_cb_t cb, void *data, bool periodic);
DEF_NOT_IMPL_FUNC(ni_clear_expiration, timer_comp_t *t);

int timer_component_init(timer_comp_t *t, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, type, init, deinit) < 0) {
        error("Failed to initialize '%s' timer component", bcomp->id);
        return -1;
    }

    t->ops.get_value = ni_get_value;
    t->ops.set_value = ni_set_value;
    t->ops.get_remaining = ni_get_remaining;
    t->ops.reset = ni_reset;
    t->ops.set_enable = ni_set_enable;
    t->ops.set_expiration_ticks = ni_set_expiration_ticks;
    t->ops.clear_expiration = ni_clear_expiration;

    board_get_bool_attr_def(bcomp, "intio", &t->intio, false);
    if (t->intio) {
        int irq;
        if (board_get_int_attr(bcomp, "irq", &irq) < 0 || irq < 0) {
            error("Invalid or no irq was specified in the board info");
            return -1;
        }
        t->irq = irq;
        board_get_irq_trigger_attr_def(bcomp, "trigger", &t->irq_trigger, IRQ_TRIGGER_LEVEL_HIGH);

        if (board_get_component_attr(bcomp, "intc", (component_t **) &t->intc) < 0) {
            error("invalid or no interrupt controller specified in the board info");
            return -1;
        }
    }

    board_get_int_attr_def(bcomp, "maxfreq", (int *) &t->maxfreq, 0);
    t->curfreq = t->maxfreq;

    return 0;
}

static int curfreq_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    timer_comp_t *t = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", t->curfreq);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int irq_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    timer_comp_t *t = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%u", t->irq);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int curtimer_ticks_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    timer_comp_t *t = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", t->curtimer.enabled ? (uint32_t) t->curtimer.ticks : 0);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int curtimer_periodic_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    timer_comp_t *t = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%d", t->curtimer.enabled ? t->curtimer.periodic : 0);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int create_instance_sysfs(timer_comp_t *t) {
    fs_dentry_t *root = vfs_dentry_lookup_from(_laritos.fs.comp_type_root, "timer");
    fs_dentry_t *dir = vfs_dir_create(root, t->parent.id, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (dir == NULL) {
        error("Error creating '%s' sysfs directory", t->parent.id);
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "curfreq", curfreq_read, t) == NULL) {
        error("Failed to create 'irqcount' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "irq", irq_read, t) == NULL) {
        error("Failed to create 'irq' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "curtimer_ticks", curtimer_ticks_read, t) == NULL) {
        error("Failed to create 'curtimer_ticks' sysfs file");
        return -1;
    }
    if (pseudofs_create_custom_ro_file_with_dataptr(dir, "curtimer_periodic", curtimer_periodic_read, t) == NULL) {
        error("Failed to create 'curtimer_periodic' sysfs file");
        return -1;
    }

    return 0;
}

int timer_component_register(timer_comp_t *t) {
    if (component_register((component_t *) t) < 0) {
        error("Couldn't register '%s'", t->parent.id);
        return -1;
    }
    create_instance_sysfs(t);
    return 0;
}

SYSFS_COMPONENT_TYPE_MODULE(timer)
