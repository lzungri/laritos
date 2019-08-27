#pragma once

#include <component.h>

struct chardev;
typedef struct {
    int (*write)(struct chardev *cd, const void *buf, size_t n);
    int (*read)(struct chardev *cd, void *buf, size_t n);
} chardev_ops_t;

typedef struct chardev {
    component_t parent;

    chardev_ops_t ops;
} chardev_t;

int chardev_init(chardev_t *uart, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n));
int chardev_register(chardev_t *cd);
