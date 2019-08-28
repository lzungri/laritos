#include <log.h>
#include <chardev.h>
#include <component.h>
#include <board.h>
#include <printf.h>


int chardev_init(chardev_t *cd, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n)) {

    char cdid[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(cdid, sizeof(cdid) - 1, "chardev@%s", bcomp->id);

    if (component_init((component_t *) cd, cdid, bcomp, COMP_TYPE_CHARDEV, init, deinit) < 0) {
        error("Failed to initialize '%s' chardev component", bcomp->id);
        return -1;
    }
    cd->ops.read = read;
    cd->ops.write = write;
    return 0;
}
