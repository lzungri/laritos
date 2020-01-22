#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <irq/irq.h>
#include <generated/autoconf.h>

typedef struct {
    char *name;
    char *value;
} board_comp_attr_t;

typedef struct {
    char *id;
    char *driver;
    board_comp_attr_t attr[CONFIG_BOARD_MAX_COMPONENT_ATTRS];
    uint8_t attrlen;
    bool processed;
} board_comp_t;

typedef struct {
    board_comp_t components[CONFIG_BOARD_MAX_COMPONENTS];
    uint8_t len;
} board_info_t;

typedef struct {
    char *name;
    int (*board_init)(board_info_t *binfo);
} board_t;

// Variable holding the plain-text board information (generated via `ld -r -b binary`)
extern char _binary_boardinfo_start[];

#define BOARD(_name, _binit) \
    board_t BOARD = { \
        .name = (_name), \
        .board_init = (_binit), \
    }

extern board_t BOARD;
