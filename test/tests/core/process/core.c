#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <printf.h>
#include <dstruct/list.h>
#include <test/test.h>
#include <process/core.h>
#include <time/time.h>
#include <component/vrtimer.h>
#include <generated/autoconf.h>
#include <test/utils.h>


static bool is_process_active(pcb_t *pcb) {
    return is_process_in(&pcb->sched.pcb_node, &_laritos.proc.pcbs);
}

static int kproc0(void *data) {
    sleep(3);
    bool *finish = (bool *) data;
    *finish = true;
    return 12345;
}

T(process_kernel_process_exit_status_matches_kfunc_t_return_value) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high prio 1", kproc0, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_active(p1));

    while (!finish1);

    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p1->exit_status == 12345);
    debug("Kernel thread finished");
TEND


static int kproc1(void *data) {
    TEST_BUSY_WAIT(3);

    bool *finish = (bool *) data;
    *finish = true;
    return 0;
}

T(process_spawning_kernel_process_with_highest_priority_switches_to_it) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high prio 1", kproc1, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_active(p1));

    while (!finish1);

    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    debug("Kernel thread finished");
TEND


static int kproc2(void *data) {
    int i;
    for (i = 1; i < 4; i++) {
        sleep(i);
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
    tassert(is_process_active(p1));

    bool finish2 = false;
    pcb_t *p2 = process_spawn_kernel_process("higher", kproc2, &finish2,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL + 1);
    tassert(p2 != NULL);
    tassert(is_process_active(p2));

    bool finish3 = false;
    pcb_t *p3 = process_spawn_kernel_process("highest", kproc2, &finish3,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p3 != NULL);
    tassert(is_process_active(p3));

    while (!finish1 || !finish2 || !finish3);

    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p2->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p3->sched.status == PROC_STATUS_ZOMBIE);
TEND

static int kproc3(void *data) {
    TEST_BUSY_WAIT(5);

    bool *finish = (bool *) data;
    *finish = true;
    return 0;
}

T(process_round_robin_on_high_priority_kernel_threads) {
    bool finish1 = false;
    pcb_t *p1 = process_spawn_kernel_process("high", kproc3, &finish1,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_active(p1));

    bool finish2 = false;
    pcb_t *p2 = process_spawn_kernel_process("high", kproc3, &finish2,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p2 != NULL);
    tassert(is_process_active(p2));

    bool finish3 = false;
    pcb_t *p3 = process_spawn_kernel_process("high", kproc3, &finish3,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p3 != NULL);
    tassert(is_process_active(p3));

    while (!finish1 || !finish2 || !finish3);

    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p2->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p3->sched.status == PROC_STATUS_ZOMBIE);
TEND

static int test_child(void *data) {
    bool *terminate = (bool *) data;
    while (!*terminate) {
        sleep(1);
    }
    return 0;
}

T(process_new_kernel_process_is_child_of_test_process) {
    bool terminate = false;
    pcb_t *child = process_spawn_kernel_process("testchild", test_child, &terminate,
                        8196, process_get_current()->sched.priority);
    tassert(child != NULL);
    tassert(is_process_active(child));
    tassert(child->parent == process_get_current());
    tassert(is_process_in(&child->siblings, &process_get_current()->children));

    terminate = true;
    while (child->sched.status != PROC_STATUS_ZOMBIE);
TEND

static int test_child1(void *data) {
    bool *terminate = (bool *) data;
    while (!*terminate) {
        sleep(1);
    }
    return 0;
}

static int test_parent1(void *data) {
    bool terminate_child = false;
    pcb_t *child = process_spawn_kernel_process("test_child", test_child1, &terminate_child,
                        8196, process_get_current()->sched.priority);
    tassert(child != NULL);
    tassert(is_process_active(child));
    tassert(child->parent == process_get_current());
    tassert(is_process_in(&child->siblings, &process_get_current()->children));

    bool *terminate = (bool *) data;
    while (!*terminate) {
        sleep(1);
    }

    terminate_child = true;
    while (child->sched.status != PROC_STATUS_ZOMBIE);

    return 0;
}

T(process_kernel_proc_spawning_new_proc_becomes_its_parent) {
    bool terminate = false;
    pcb_t *parent = process_spawn_kernel_process("test_parent", test_parent1, &terminate,
                        8196, process_get_current()->sched.priority);
    tassert(parent != NULL);
    tassert(is_process_active(parent));

    terminate = true;
    while (parent->sched.status != PROC_STATUS_ZOMBIE);
TEND

static int test_child2(void *data) {
    bool *terminate = (bool *) data;
    while (!*terminate) {
        sleep(1);
    }
    return 0;
}

static int test_parent2(void *data) {
    bool *terminate_child = (bool *) data;
    pcb_t *child = process_spawn_kernel_process("test_child", test_child2, terminate_child,
                        8196, process_get_current()->sched.priority);
    return child->pid;
}

T(process_orphan_proc_grandparent_becomes_new_parent) {
    bool terminate_child = false;
    pcb_t *parent = process_spawn_kernel_process("test_parent", test_parent2, &terminate_child,
                        8196, process_get_current()->sched.priority);
    tassert(parent != NULL);
    tassert(is_process_active(parent));

    while (parent->sched.status != PROC_STATUS_ZOMBIE) {
        sleep(1);
    }

    uint16_t child_pid = parent->exit_status;
    pcb_t *pcb = NULL;
    for_each_process(pcb) {
        if (pcb->pid == child_pid) {
            break;
        }
    }

    tassert(pcb != NULL);
    tassert(is_process_active(pcb));
    tassert(pcb->parent == process_get_current());
    tassert(is_process_in(&pcb->siblings, &process_get_current()->children));

    terminate_child = true;
    while (pcb->sched.status != PROC_STATUS_ZOMBIE);
TEND

static int blocked(void *data) {
    sleep(10);
    return 0;
}

T(process_blocked_state_stats_are_accurate) {
    pcb_t *p = process_spawn_kernel_process("blocked", blocked, NULL,
                        8196, process_get_current()->sched.priority);
    tassert(p != NULL);
    tassert(is_process_active(p));

    while (p->sched.status != PROC_STATUS_ZOMBIE) {
        sleep(1);
    }

    vrtimer_comp_t *vrtimer = (vrtimer_comp_t *) component_first_of_type(COMP_TYPE_VRTIMER);
    tassert(vrtimer != NULL);
    timer_comp_t *hrtimer = vrtimer->hrtimer;
    tassert(hrtimer != NULL);

    uint8_t secs = TICK_TO_SEC(hrtimer, p->stats.ticks_spent[PROC_STATUS_BLOCKED]);
    tassert(secs >= 10 && secs <= 11);
TEND

static int ready(void *data) {
    return 0;
}

T(process_ready_state_stats_are_accurate) {
    // Low priority, it will not run even if the test process is blocked
    pcb_t *p = process_spawn_kernel_process("ready", ready, NULL,
                        8196, CONFIG_SCHED_PRIORITY_LOWEST);
    tassert(p != NULL);
    tassert(is_process_active(p));

    // We need to keep the test process running, otherwise the scheduler will switch to the ready process
    TEST_BUSY_WAIT(5);
    process_kill(p);

    vrtimer_comp_t *vrtimer = (vrtimer_comp_t *) component_first_of_type(COMP_TYPE_VRTIMER);
    tassert(vrtimer != NULL);
    timer_comp_t *hrtimer = vrtimer->hrtimer;
    tassert(hrtimer != NULL);

    uint8_t secs = TICK_TO_SEC(hrtimer, p->stats.ticks_spent[PROC_STATUS_READY]);
    tassert(secs >= 4 && secs <= 6);
TEND

static int running(void *data) {
    while(1);
    return 0;
}

T(process_running_state_stats_are_accurate) {
    // Lower priority than the test process. It will only run when the test process is blocked
    pcb_t *p = process_spawn_kernel_process("running", running, NULL,
                        8196,  process_get_current()->sched.priority + 1);
    tassert(p != NULL);
    tassert(is_process_active(p));

    sleep(5);
    process_kill(p);

    vrtimer_comp_t *vrtimer = (vrtimer_comp_t *) component_first_of_type(COMP_TYPE_VRTIMER);
    tassert(vrtimer != NULL);
    timer_comp_t *hrtimer = vrtimer->hrtimer;
    tassert(hrtimer != NULL);

    uint8_t secs = TICK_TO_SEC(hrtimer, p->stats.ticks_spent[PROC_STATUS_RUNNING]);
    tassert(secs >= 5 && secs <= 6);
TEND

static int waitfor_exitstatus(void *data) {
    return 12345;
}

T(process_wait_for_process_returns_exit_status_of_dead_process) {
    pcb_t *p = process_spawn_kernel_process("exitstatus", waitfor_exitstatus, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p != NULL);
    int status;
    process_wait_for(p, &status);
    tassert(status == 12345);
TEND

T(process_wait_for_pid_returns_exit_status_of_dead_process) {
    pcb_t *p = process_spawn_kernel_process("exitstatus", waitfor_exitstatus, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p != NULL);
    int status;
    process_wait_pid(p->pid, &status);
    tassert(status == 12345);
TEND

T(process_can_only_wait_for_children) {
    int status;
    // Waiting for init process
    tassert(process_wait_pid(0, &status) < 0);
TEND

static int waitfor(void *data) {
    sleep(3);
    return 12345;
}

T(process_wait_for_process_blocks_until_proc_is_dead) {
    pcb_t *p = process_spawn_kernel_process("waitfor", waitfor, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p != NULL);
    int status;
    process_wait_for(p, &status);
    tassert(status == 12345);
TEND

static int wait1(void *data) {
    sleep(3);
    return 12345;
}

static int wait2(void *data) {
    pcb_t *p1 = process_spawn_kernel_process("wait1", wait1, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    int status;
    process_wait_for(p1, &status);
    return status - 5;
}

T(process_multiple_processes_are_blocked_if_waiting_for_another_one_to_die) {
    pcb_t *p2 = process_spawn_kernel_process("wait2", wait2, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p2 != NULL);

    int status;
    process_wait_for(p2, &status);
    tassert(status == 12340);
TEND

static int procn(void *data) {
    pcb_t *cur = process_get_current();
    // Sleep for "random" seconds
    sleep(cur->pid % 7 + 1);
    return cur->pid;
}

T(process_spawning_lots_of_processes_doesnt_crash_the_system) {
    pcb_t *procs[CONFIG_PROCESS_MAX_CONCURRENT_PROCS - 10];
    int i;
    for (i = 0; i < ARRAYSIZE(procs); i++) {
        char buf[CONFIG_PROCESS_MAX_NAME_LEN] = { 0 };
        snprintf(buf, sizeof(buf), "p%d", i);

        procs[i] = process_spawn_kernel_process(buf, procn, NULL,
                            8196,  process_get_current()->sched.priority);
        tassert(procs[i] != NULL);
    }

    for (i = 0; i < ARRAYSIZE(procs); i++) {
        uint16_t pid = procs[i]->pid;
        debug("waiting for pid=%u", pid);
        int status;
        process_wait_for(procs[i], &status);
        // We can no longer use the pcb_t since it was probably garbage collected
        procs[i] = NULL;
        tassert(status == pid);
    }
TEND
