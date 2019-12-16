#include <log.h>

#include <stdint.h>
#include <test/test.h>
#include <process/core.h>
#include <generated/autoconf.h>


#include <sched/core.h>

static int highprio(void *data) {
    unsigned long long i = 0;
    info("HIGH PRIO");
    while (i++ < 10000000) {
        if (i % 1000000 == 0)  {
            schedule();
        }
    }
    return 0;
}

T(process_spawning_kernel_process_with_highest_priority_switches_to_it) {
    pcb_t *p = process_spawn_kernel_process("high prio", highprio, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p != NULL);

    p = process_spawn_kernel_process("high prio 2", highprio, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p != NULL);

//    unsigned long long i = 0;
//    while (i++ < 1000000000);
    unsigned int i = 0;
    while (i++ < 1000000000) {
//    while(1) {
        if (i++ % 100000000 == 0)  {
            info("TEST");
        }
    }
TEND
