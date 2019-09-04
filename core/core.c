#include <log.h>
#include <core.h>
#include <board.h>
#include <string.h>
#include <component.h>
#include <inputdev.h>
#include <generated/utsrelease.h>


laritos_t _laritos;

void kernel_entry(void)  {
    log("I", "-- laritOS " UTS_RELEASE " --");
    info("Initializing kernel");

    if (board_parse_and_initialize(&_laritos.bi) < 0) {
        fatal("Couldn't initialize board");
    }

#ifdef CONFIG_LOG_LEVEL_DEBUG
    board_dump_board_info(&_laritos.bi);
#endif

    if (driver_process_board_components(&_laritos.bi) < 0) {
        fatal("Error processing board components");
    }

#ifdef CONFIG_LOG_LEVEL_DEBUG
    component_dump_registered_comps();
#endif

    while (true) {
        component_t *c;
        for_each_filtered_component(c, c->type == COMP_TYPE_INPUTDEV) {
            stream_t *s = ((inputdev_t *) c)->transport;
            char buf[32];
            int bread = 0;
            if ((bread = s->ops.read(s, buf, sizeof(buf) - 1)) > 0) {
                buf[bread] = '\0';
                info("[%s]: %s", c->id, buf);


                switch (buf[0]) {
                case 's':
                    asm("svc 1");
                    break;
                case 'r':
                    asm("b 0");
                    break;
                }
            }
        }
    }
}
