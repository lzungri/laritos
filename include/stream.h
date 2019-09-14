#pragma once

#include <component.h>
#include <stdbool.h>

struct stream;
typedef struct {
    int (*write)(struct stream *cd, const void *buf, size_t n);
    int (*read)(struct stream *cd, void *buf, size_t n);
} stream_ops_t;

typedef struct stream {
    component_t parent;

    stream_ops_t ops;
    bool blocking;
} stream_t;

int stream_component_init(stream_t *stream, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n));
