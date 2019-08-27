#include <log.h>
#include <chardev.h>
#include <component.h>
#include <board.h>


int chardev_init(chardev_t *cd, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n)) {

    if (component_init((component_t *) cd, bcomp, type == COMP_TYPE_UNKNOWN ? COMP_TYPE_CHARDEV : type, init, deinit) < 0) {
        error("Failed to initialize '%s' chardev component", bcomp->id);
        return -1;
    }
    cd->ops.read = read;
    cd->ops.write = write;
    return 0;
}

int chardev_register(chardev_t *cd) {
    return component_register((component_t *) cd);
}
