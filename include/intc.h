#pragma once

#include <component.h>

typedef struct {
    component_t parent;


} intc_t;

int intc_init(intc_t *comp, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
