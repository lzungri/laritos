#pragma once

#include <component.h>
#include <board.h>
#include <stream.h>

typedef struct inputdev {
    component_t parent;

    stream_t *transport;
} inputdev_t;
