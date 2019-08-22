#include <log.h>
#include <driver.h>
#include <board.h>

static int process(board_comp_t *comp) {
    return 0;
}

DEF_DRIVER_MANAGER(test, process);
