#include <log.h>
#include <component.h>
#include <board.h>
#include <printf.h>
#include <stream.h>


int stream_init(stream_t *s, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n)) {

    char cdid[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(cdid, sizeof(cdid), "stream@%s", bcomp->id);

    if (component_init((component_t *) s, cdid, bcomp, COMP_TYPE_STREAM, init, deinit) < 0) {
        error("Failed to initialize '%s' stream component", bcomp->id);
        return -1;
    }
    s->ops.read = read;
    s->ops.write = write;

    board_get_bool_attr(bcomp, "blocking", &s->blocking, false);
    return 0;
}
