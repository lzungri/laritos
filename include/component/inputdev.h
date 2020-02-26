#pragma once

#include <board/types.h>
#include <component/component.h>
#include <component/stream.h>

typedef struct inputdev {
    component_t parent;

    stream_t *transport;
} inputdev_t;

int inputdev_component_init(inputdev_t *input, char *id, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int inputdev_component_register(inputdev_t *input);
