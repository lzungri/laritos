#pragma once

//#include <stdint.h>

#define BOARD_MAX_COMPONENTS 10
#define BOARD_MAX_COMPONENT_ATTRS 10

typedef struct {
    char *name;
    char *value;
} comp_attr_info_t;

typedef struct {
    char *name;
    char *driver;
    comp_attr_info_t attr[BOARD_MAX_COMPONENT_ATTRS];
    int attrlen;
} comp_info_t;

typedef struct {
    comp_info_t components[BOARD_MAX_COMPONENTS];
    int len;
} board_info_t;

typedef struct {
    char *name;
    int (*board_init)(board_info_t *binfo);
} board_t;

#define BOARD(_name, _binit) \
    board_t BOARD = { \
        .name = _name, \
        .board_init = _binit, \
    }

extern board_t BOARD;

int board_init(board_info_t *binfo);
int board_parse_info(char *bi_start_addr, board_info_t *bi);
