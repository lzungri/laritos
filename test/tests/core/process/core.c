#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <test/test.h>
#include <process/core.h>
#include <generated/autoconf.h>


static bool is_process_in(pcb_t *pcb) {
    pcb_t *p;
    for_each_process(p) {
        if (p == pcb) {
            return true;
        }
    }
    return false;
}


static int kproc0(void *data) {
    unsigned long long i = 0;
    while (i++ < 100000);

    bool *finish = (bool *) data;
    *finish = true;
    return 12345;
}

T(process_kernel_process_exit_status_matches_kfunc_t_return_value) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high prio 1", kproc0, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_in(p1));

    while (!finish1);

    tassert(p1->sched.status == PCB_STATUS_ZOMBIE);
    tassert(p1->exit_status == 12345);
    debug("Kernel thread finished");

    process_unregister(p1);
    tassert(!is_process_in(p1));
TEND


static int kproc1(void *data) {
    unsigned long long i = 0;
    while (i++ < 1000) {
        verbose("%s: %lu", process_get_current()->name, (uint32_t) i);
    }

    bool *finish = (bool *) data;
    *finish = true;
    return 0;
}

T(process_spawning_kernel_process_with_highest_priority_switches_to_it) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high prio 1", kproc1, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_in(p1));

    while (!finish1);

    tassert(p1->sched.status == PCB_STATUS_ZOMBIE);
    debug("Kernel thread finished");

    process_unregister(p1);
    tassert(!is_process_in(p1));
TEND


static int kproc2(void *data) {
    unsigned long long i = 0;
    while (i++ < 2000 * process_get_current()->pid) {
        verbose("%s: %lu", process_get_current()->name, (uint32_t) i);
    }

    bool *finish = (bool *) data;
    *finish = true;
    return 0;
}

T(process_ready_kernel_threads_with_highest_priority_is_executed_first) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high", kproc2, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL + 2);
    tassert(p1 != NULL);
    tassert(is_process_in(p1));

    bool finish2 = false;
    pcb_t *p2 = process_spawn_kernel_process("higher", kproc2, &finish2,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL + 1);
    tassert(p2 != NULL);
    tassert(is_process_in(p2));

    bool finish3 = false;
    pcb_t *p3 = process_spawn_kernel_process("highest", kproc2, &finish3,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p3 != NULL);
    tassert(is_process_in(p3));

    while (!finish1 || !finish2 || !finish3);

    tassert(p1->sched.status == PCB_STATUS_ZOMBIE);
    tassert(p2->sched.status == PCB_STATUS_ZOMBIE);
    tassert(p3->sched.status == PCB_STATUS_ZOMBIE);
    debug("Kernel threads finished");

    process_unregister(p1);
    tassert(!is_process_in(p1));
    process_unregister(p2);
    tassert(!is_process_in(p2));
    process_unregister(p3);
    tassert(!is_process_in(p3));
TEND
