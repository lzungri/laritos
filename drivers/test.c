#include <log.h>
#include <driver.h>
#include <board.h>

static int init(board_comp_t *comp) {
    return 0;
}

static int deinit(void) {
    return 0;
}

static driver_t _driver_test = {
    .name = "test",
    .init = init,
    .deinit = deinit,
};

driver_t *_driver_test_ptr __attribute__ ((section (".drivers"))) = &_driver_test;
