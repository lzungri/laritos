#pragma once

#include <board.h>
#include <board-types.h>
#include <component/component.h>
#include <stdbool.h>

struct stream;
typedef struct {
    int (*write)(struct stream *s, const void *buf, size_t n);
    int (*read)(struct stream *s, void *buf, size_t n);
} stream_ops_t;

typedef struct stream {
    component_t parent;

    stream_ops_t ops;
    bool blocking;
} stream_t;

static inline int stream_component_init(stream_t *s, board_comp_t *bcomp, char *id,
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n)) {
    if (component_init((component_t *) s, id, bcomp, COMP_TYPE_STREAM, NULL, NULL) < 0) {
        error("Failed to initialize '%s' stream component", bcomp->id);
        return -1;
    }
    s->ops.read = read;
    s->ops.write = write;

    board_get_bool_attr_def(bcomp, "blocking", &s->blocking, false);
    return 0;
}
