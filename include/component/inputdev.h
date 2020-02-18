#pragma once

#include <board/types.h>
#include <component/component.h>
#include <component/stream.h>

typedef struct inputdev {
    component_t parent;

    stream_t *transport;
} inputdev_t;
