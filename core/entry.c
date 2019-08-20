#include <entry.h>
#include <board.h>
#include <log.h>
#include <string.h>


static void loop(void) {
	while (1) {
		asm("wfi");
	}
}

static int initialize_board(void) {
    board_info_t bi = { 0 };
    debug("Parsing board info data");

    if (board_parse_info(_binary_boardinfo_start, &bi) < 0) {
        error("Error parsing board info");
        return -1;
    }

    info("Initializing %s board", BOARD.name);
    if (BOARD.board_init(&bi) < 0) {
        error("Failed to initialize board");
        return -1;
    }

    return 0;
}

void kernel_entry(void)  {
    info("Initializing kernel");

    if (initialize_board() < 0) {
        fatal("Couldn't initialize board");
    }

    loop();
}
