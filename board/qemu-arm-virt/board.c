#include <log.h>
#include <board/board-types.h>
#include <board/board.h>


static int qemu_board_init(board_info_t *bi) {
    info("qemu board custom initialization");

    return board_init(bi);
}

BOARD("qemu-arm-virt", qemu_board_init);
