#include <log.h>
#include <core.h>
#include <string.h>
#include <component/component.h>
#include <component/inputdev.h>
#include <component/timer.h>
#include <timer.h>
#include <board-types.h>
#include <board.h>
#include <utils/debug.h>
#include <generated/utsrelease.h>


laritos_t _laritos;

static void shell(void) {
    while (true) {
        component_t *c;
        for_each_filtered_component(c, c->type == COMP_TYPE_INPUTDEV) {
            stream_t *s = ((inputdev_t *) c)->transport;
            char buf[32];
            int bread = 0;
            if ((bread = s->ops.read(s, buf, sizeof(buf) - 1, true)) > 0) {
                buf[bread] = '\0';
                info("[%s]: %s", c->id, buf);


                switch (buf[0]) {
                case 's':
                    // system call
                    asm("mov r0, %[c]" : : [c] "r" (buf[0]));
                    asm("mov r1, #2");
                    asm("mov r2, #3");
                    asm("mov r3, #4");
                    asm("mov r4, #5");
                    asm("mov r5, #6");
                    asm("mov r6, #7");
                    asm("svc 1");
                    break;
                case 'r':
                    // reset
                    asm("b 0");
                    break;
                case 'd':
                    // data abort exc
                    asm("movw r7, #0xffff");
                    asm("movt r7, #0xffff");
                    asm("str r6, [r7]");
                    break;
                case 'p':
                    // prefetch exc
                    asm("movw r7, #0xffff");
                    asm("movt r7, #0xffff");
                    asm("mov pc, r7");
                    break;
                case 'u':
                    // undef exc
                    asm(".word 0xffffffff");
                    break;
                case 't':;
                    // rtc timer status
                    component_t *c1;
                    for_each_filtered_component(c1, c1->type == COMP_TYPE_RTC) {
                        timer_comp_t *t = (timer_comp_t *) c1;
                        int64_t v;
                        t->ops.get_value(t, &v);
                        log_always("rtc value: %lu", (uint32_t) v);
                        t->ops.get_remaining(t, &v);
                        log_always("rtc remaining: %ld", (int32_t) v);
                    }
                    break;
                case 'e':;
                    // rtc timer expiration
                    component_t *c2;
                    for_each_filtered_component(c2, c2->type == COMP_TYPE_RTC) {
                        timer_comp_t *t = (timer_comp_t *) c2;
                        t->ops.set_expiration(t, 5, 0, TIMER_EXP_RELATIVE);
                    }
                    break;
                }
            }
        }
    }
}

void kernel_entry(void)  {
    log_always("-- laritOS " UTS_RELEASE " --");
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
    dump_registered_comps();
#endif

    shell();
}
