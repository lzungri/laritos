#include <core.h>
#include <board.h>
#include <log.h>
#include <string.h>
#include <generated/utsrelease.h>


laritos_t _laritos;

static void loop(void) {
	while (1) {
		asm("wfi");
	}
}


void kernel_entry(void)  {
    info("-- laritOS " UTS_RELEASE " --");
    info("Initializing kernel");

    if (board_parse_and_initialize(&_laritos.bi) < 0) {
        fatal("Couldn't initialize board");
    }

#ifdef CONFIG_LOG_LEVEL_DEBUG
    debug("TODO: Dump board info struct");
#endif

    if (driver_process_board_components(&_laritos.bi) < 0) {
        fatal("Error processing board components");
    }

    loop();
}
