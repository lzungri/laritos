#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <component/component.h>
#include <component/intc.h>
#include <generated/autoconf.h>

struct blockdev;
typedef struct {
    int (*read)(struct blockdev *blk, void *buf, size_t n, uint32_t offset);
    int (*write)(struct blockdev *blk, void *buf, size_t n, uint32_t offset);
} blockdev_ops_t;

typedef struct blockdev {
    component_t parent;

    uint32_t nsectors;
    uint32_t sector_size;
    uint32_t size_kbs;
    blockdev_ops_t ops;
} blockdev_t;

int blockdev_init(blockdev_t *blk);
int blockdev_deinit(blockdev_t *blk);

int blockdev_component_init(blockdev_t *blk, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(blockdev_t *blk, void *buf, size_t n, uint32_t offset),
        int (*write)(blockdev_t *blk, void *buf, size_t n, uint32_t offset));
int blockdev_component_register(blockdev_t *blk);
