#include <log.h>
#include <board-types.h>
#include <board.h>


static int qemu_board_init(board_info_t *bi) {
    info("qemu board custom initialization");

    return board_init(bi);
}

BOARD("qemu-arm-virt", qemu_board_init);
