#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <test/test.h>
#include <process/core.h>
#include <generated/autoconf.h>


static int highprio(void *data) {
    info("%s", pcb_get_current()->name);

    unsigned long long i = 0;
    while (i++ < 10000000);

    bool *finish = (bool *) data;
    *finish = true;
    return 0;
}

T(process_spawning_kernel_process_with_highest_priority_switches_to_it) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high prio", highprio, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);

    bool finish2 = false;
    pcb_t *p2 = process_spawn_kernel_process("high prio 2", highprio, &finish2,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p2 != NULL);

    bool finish3 = false;
    pcb_t *p3 = process_spawn_kernel_process("high prio 3", highprio, &finish3,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p3 != NULL);

    while (!finish1 || !finish2 || !finish3);
    info("Kernel threads finished");

    process_unregister(p1);
    process_unregister(p2);
    process_unregister(p3);
TEND
