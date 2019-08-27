#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <generated/autoconf.h>

#define BOARD_MAX_ATTR_NAME_LEN_BYTES 32
#define BOARD_MAX_ATTR_VALUE_LEN_BYTES 64
#define BOARD_MAX_COMP_ID_LEN_BYTES BOARD_MAX_ATTR_VALUE_LEN_BYTES

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
    board_comp_t components[CONFIG_MAX_COMPONENTS];
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
        .name = _name, \
        .board_init = _binit, \
    }

extern board_t BOARD;

int board_init(board_info_t *binfo);
int board_parse_info(char *bi_start_addr, board_info_t *bi);
int board_parse_and_initialize(board_info_t *bi);

int board_get_ptr_attr(board_comp_t *bc, char *attr, void **buf, void *def);
int board_get_int_attr(board_comp_t *bc, char *attr, int *buf, int def);
int board_get_str_attr(board_comp_t *bc, char *attr, char *buf, char *def);
int board_get_str_attr_idx(board_comp_t *bc, char *attr, char *buf, uint8_t index, char *def);
