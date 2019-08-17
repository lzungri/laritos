#define MODULE "core.entry"

#include <entry.h>
#include <board.h>
#include <log.h>

static void loop(void) {
	while (1) {
		asm("wfi");
	}
}

// TODO Implement these functions
int memset(void *addr, char v, int len) {
    return 0;
}

static char __board_info_start[] =
        "cpu:armv7-a|smp=4,iset=le\n"
        "uart:uart-arm|addr=0x500,ctrladdr=0x300\n"
        "hd:hd-generic|blocks=100\n"
        "touch:synaptics\n\n\n"
        "accel:bosch|odr=1600\n\n";

static int initialize_board(void) {
    board_info_t bi = { 0 };
    debug("Parsing board info data");

    if (board_parse_info(__board_info_start, &bi) < 0) {
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
