#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <printf.h>
#include <board/core.h>
#include <utils/function.h>
#include <component/cpu.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>

DEF_NOT_IMPL_FUNC(ni_set_irqs_enable, cpu_t *c, bool enabled);


static int freq_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    cpu_t *cpu = f->data0;
    char data[16];
    int strlen = snprintf(data, sizeof(data), "%lu", (uint32_t) cpu->freq);
    return pseudofs_write_to_buf(buf, blen, data, strlen + 1, offset);
}

static int schedid_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    cpu_t *cpu = f->data0;
    char *id = ((component_t *) cpu->sched)->id;
    return pseudofs_write_to_buf(buf, blen, id, COMPONENT_MAX_ID_LEN, offset);
}

static int create_instance_sysfs(cpu_t *c) {
    fs_dentry_t *cpu_root = vfs_dentry_lookup_from(_laritos.fs.comp_type_root, "cpu");
    fs_dentry_t *cpudir = vfs_dir_create(cpu_root, c->parent.id, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    if (cpudir == NULL) {
        error("Error creating '%s' sysfs directory", c->parent.id);
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(cpudir, "freq", freq_read, c) == NULL) {
        error("Failed to create 'freq' sysfs file");
        return -1;
    }

    if (pseudofs_create_custom_ro_file_with_dataptr(cpudir, "schedid", schedid_read, c) == NULL) {
        error("Failed to create 'schedid' sysfs file");
        return -1;
    }
    return 0;
}

int cpu_component_init(cpu_t *c, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) c, bcomp->id, bcomp, COMP_TYPE_CPU, init, deinit) < 0) {
        error("Failed to initialize '%s' cpu component", bcomp->id);
        return -1;
    }

    c->ops.set_irqs_enable = ni_set_irqs_enable;

    if (board_get_int_attr(bcomp, "id", (int *) &c->id) < 0) {
        error("invalid or no cpu id specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "intc", (component_t **) &c->intc) < 0) {
        error("invalid or no interrupt controller specified in the board info");
        return -1;
    }

    if (board_get_component_attr(bcomp, "sched", (component_t **) &c->sched) < 0) {
        error("invalid or no scheduler specified in the board info");
        return -1;
    }

    if (board_get_u64_attr(bcomp, "freq", &c->freq) < 0) {
        error("invalid or no cpu frequency specified in the board info");
        return -1;
    }

    return 0;
}

int cpu_component_register(cpu_t *c) {
    if (component_register((component_t *) c) < 0) {
        error("Couldn't register cpu '%s'", c->parent.id);
        return -1;
    }

    // Save CPU shortcut
    _laritos.cpu[c->id] = (cpu_t *) c;

    create_instance_sysfs(c);
    return 0;
}

SYSFS_COMPONENT_TYPE_MODULE(cpu)
