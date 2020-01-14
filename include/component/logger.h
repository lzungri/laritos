#pragma once

#include <stdbool.h>
#include <component/component.h>
#include <component/stream.h>

struct logger_comp;
typedef struct {
    int (*write)(struct logger_comp *l, const void *buf, size_t n, bool blocking);
} logger_comp_ops_t;

typedef struct logger_comp {
    component_t parent;

    stream_t *transport;
    logger_comp_ops_t ops;
} logger_comp_t;
