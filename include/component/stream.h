#pragma once

#include <board.h>
#include <board-types.h>
#include <component/component.h>
#include <stdbool.h>

struct stream;
typedef struct {
    int (*write)(struct stream *s, const void *buf, size_t n, bool blocking);
    int (*read)(struct stream *s, void *buf, size_t n, bool blocking);
} stream_ops_t;

typedef struct stream {
    component_t parent;

    stream_ops_t ops;
} stream_t;

static inline int stream_component_init(stream_t *s, board_comp_t *bcomp, char *id, component_type_t type,
        int (*read)(stream_t *s, void *buf, size_t n, bool b), int (*write)(stream_t *s, const void *buf, size_t n, bool b)) {
    if (component_init((component_t *) s, id, bcomp, type, NULL, NULL) < 0) {
        error("Failed to initialize '%s' stream component", bcomp->id);
        return -1;
    }
    s->ops.read = read;
    s->ops.write = write;
    return 0;
}
