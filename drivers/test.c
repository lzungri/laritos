#include <log.h>
#include <driver.h>
#include <board.h>

static int process(board_comp_t *comp) {
    return 0;
}

static driver_mgr_t _driver_mgr_test = {
    .name = "test",
    .process = process,
};

driver_mgr_t *_driver_mgr_test_ptr __attribute__ ((section (".driver_mgrs"))) = &_driver_mgr_test;
