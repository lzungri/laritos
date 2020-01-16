#include <log.h>

#include <stdbool.h>
#include <process/core.h>
#include <arch/cpu.h>
#include <component/inputdev.h>
#include <component/timer.h>
#include <component/component.h>
#include <mm/heap.h>
#include <time/system-tick.h>
#include <utils/debug.h>

int shell_main(void *data) {
    pcb_t *proc = process_get_current();

    while (true) {
        component_t *c;
        for_each_component_type(c, COMP_TYPE_INPUTDEV) {
            stream_t *s = ((inputdev_t *) c)->transport;
            char buf[32];
            int bread = 0;
            if ((bread = s->ops.read(s, buf, sizeof(buf) - 1, true)) > 0) {
                buf[bread] = '\0';
                info("[%s]: %s", c->id, buf);


                switch (buf[0]) {
                case '+':;
                    process_set_priority(proc, proc->sched.priority + 1);
                    break;
                case '-':;
                    process_set_priority(proc, proc->sched.priority - 1);
                    break;
                case '1':
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
                case '2':;
                    uint32_t *bottom = proc->mm.stack_bottom;
                    *bottom = 0xCACACACA;
                    uint32_t *top = (uint32_t *) ((char *) proc->mm.stack_bottom + proc->mm.stack_size - 4);
                    *top = 0xBABABABA;
                    break;
                case 's':
                    debug_dump_processes();
                    debug_dump_processes_stats();
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
                case 'c':;
                    calendar_t c = { 0 };
                    time_get_rtc_localtime_calendar(&c);
                    log_always("calendar: %02d/%02d/%ld %02d:%02d:%02d",
                            c.mon + 1, c.mday, c.year + 1900, c.hour, c.min, c.sec);
                    break;
                case 't':;
                    // rtc timer status
                    time_t t = { 0 };
                    time_get_rtc_time(&t);
                    log_always("rtc_gettime(): %lu", (uint32_t) t.secs);

                    component_t *c1;
                    for_each_component_type(c1, COMP_TYPE_RTC) {
                        timer_comp_t *t = (timer_comp_t *) c1;
                        int64_t v;
                        t->ops.get_remaining(t, &v);
                        log_always("rtc remaining: %ld", (int32_t) v);
                    }

                    log_always("tick_get_os_ticks(): %lu", (uint32_t) tick_get_os_ticks());
                    break;
                case 'm':;
                    char *p = malloc(10);
                    char *p2 = malloc(20);
                    char *p3 = malloc(30);
                    free(p2);
                    free(p);
                    free(p3);
                    heap_dump_info();
                    break;
                case 'h':;
                    // Heap buffer overflow
                    char *ptr = malloc(10);
                    ptr[10] = 0xCA;
                    ptr[11] = 0xCA;
                    ptr[12] = 0xCA;
                    ptr[13] = 0xCA;
                    free(ptr);
                    break;
                }
            }
        }
    }
    return 0;
}
