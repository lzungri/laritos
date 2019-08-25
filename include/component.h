#pragma once

#include <board.h>

#define MAX_COMPONENT_ID_LEN 10

typedef enum {
    COMP_TYPE_UNKNOWN = 0,
    COMP_TYPE_UART,

    COMP_TYPE_LEN,
} component_type_t;

struct component_t;
typedef struct {
    int (*init)(struct component_t *c);
    int (*deinit)(struct component_t *c);

    // TODO Power manager stuff
} component_ops_t;

typedef struct {
    char id[MAX_COMPONENT_ID_LEN];

    component_type_t type;
    component_ops_t ops;
} component_t;


int component_init(component_t *comp, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int component_register(component_t *comp);
