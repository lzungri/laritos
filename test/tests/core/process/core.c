/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <printf.h>
#include <dstruct/list.h>
#include <test/test.h>
#include <process/core.h>
#include <time/core.h>
#include <component/vrtimer.h>
#include <irq/types.h>
#include <sync/spinlock.h>
#include <generated/autoconf.h>
#include <test/utils/process.h>


static bool is_process_active(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);
    bool active = is_process_in(&pcb->sched.pcb_node, &_laritos.proc.pcbs);
    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);
    return active;
}

static int kproc0(void *data) {
    sleep(3);
    return 12345;
}

T(process_kernel_process_exit_status_matches_kfunc_t_return_value) {
    pcb_t *p1 = process_spawn_kernel_process("high prio 1", kproc0, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(is_process_active(p1));

    int status;
    process_wait_for(p1, &status);

    tassert(status == 12345);
    irqctx_t pcbdatalock_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);
    tassert(p1->exit_status == 12345);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);
TEND

static int irqproc(void *data) {
    sleep(3);
    return irq_is_enabled();
}

T(process_irqs_are_enabled_when_executing_kernel_code) {
    pcb_t *p1 = process_spawn_kernel_process("irq", irqproc, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);
    tassert(irq_is_enabled());

    int irq_enabled;
    process_wait_for(p1, &irq_enabled);
    tassert(irq_enabled);
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
    schedule();
    tassert(finish1 == true);
    process_wait_for(p1, NULL);
TEND


static int kproc2(void *data) {
    time_get_rtc_time((time_t *) data);
    TEST_BUSY_WAIT(3);
    return 0;
}

T(process_ready_kernel_threads_with_highest_priority_is_executed_first) {
    time_t t1;
    pcb_t *p1 = process_spawn_kernel_process("high", kproc2, &t1,
                        8196, process_get_current()->sched.priority + 3);
    tassert(p1 != NULL);

    time_t t2;
    pcb_t *p2 = process_spawn_kernel_process("higher", kproc2, &t2,
                        8196, process_get_current()->sched.priority + 2);
    tassert(p2 != NULL);

    time_t t3;
    pcb_t *p3 = process_spawn_kernel_process("highest", kproc2, &t3,
                        8196, process_get_current()->sched.priority + 1);
    tassert(p3 != NULL);

    process_wait_for(p1, NULL);
    process_wait_for(p2, NULL);
    process_wait_for(p3, NULL);

    tassert(t1.secs > t2.secs);
    tassert(t2.secs > t3.secs);
TEND

static int kproc3(void *data) {
    TEST_BUSY_WAIT(5);
    return 0;
}

T(process_round_robin_on_high_priority_kernel_threads) {
    pcb_t *p1 = process_spawn_kernel_process("high", kproc3, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p1 != NULL);

    pcb_t *p2 = process_spawn_kernel_process("high", kproc3, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p2 != NULL);

    pcb_t *p3 = process_spawn_kernel_process("high", kproc3, NULL,
                        8196, CONFIG_SCHED_PRIORITY_MAX_KERNEL);
    tassert(p3 != NULL);

    process_wait_for(p1, NULL);
    process_wait_for(p2, NULL);
    process_wait_for(p3, NULL);
TEND

static int test_child(void *data) {
    sleep(1);
    return 0;
}

T(process_new_kernel_process_is_child_of_test_process) {
    pcb_t *child = process_spawn_kernel_process("testchild", test_child, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(child != NULL);
    tassert(is_process_active(child));
    tassert(child->parent == process_get_current());

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    tassert(is_process_in(&child->siblings, &process_get_current()->children));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    process_wait_for(child, NULL);
TEND

static int test_child1(void *data) {
    sleep(1);
    return 0;
}

static int test_parent1(void *data) {
    pcb_t *child = process_spawn_kernel_process("test_child", test_child1, NULL,
                        8196, process_get_current()->sched.priority);
    bool *success = (bool *) data;
    *success = child->parent == process_get_current();

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    *success = *success && is_process_in(&child->siblings, &process_get_current()->children);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    process_wait_for(child, NULL);
    return 0;
}

T(process_kernel_proc_spawning_new_proc_becomes_its_parent) {
    bool success = false;
    pcb_t *parent = process_spawn_kernel_process("test_parent", test_parent1, &success,
                        8196, process_get_current()->sched.priority);
    tassert(parent != NULL);
    tassert(is_process_active(parent));

    process_wait_for(parent, NULL);
    tassert(success);
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

    int child_pid;
    process_wait_for(parent, &child_pid);

    pcb_t *pcb = NULL;

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);

    for_each_process_locked(pcb) {
        if (pcb->pid == child_pid) {
            break;
        }
    }

    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);

    tassert(pcb != NULL);
    tassert(is_process_active(pcb));
    tassert(pcb->parent == process_get_current());

    irqctx_t pcbs_data_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbs_data_ctx);
    tassert(is_process_in(&pcb->siblings, &process_get_current()->children));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbs_data_ctx);

    terminate_child = true;
    process_wait_for(pcb, NULL);
TEND

static int blocked(void *data) {
    int *i = (int *) data;
    sleep(*i);
    return 0;
}

T(process_blocked_state_stats_are_accurate) {
    int i;
    for (i = 10; i <= 30; i += 10) {
        pcb_t *p = process_spawn_kernel_process("blocked", blocked, &i,
                            8196, process_get_current()->sched.priority);
        tassert(p != NULL);
        tassert(is_process_active(p));

        process_wait_for(p, NULL);

        irqctx_t ctx;
        spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
        uint8_t secs = OSTICK_TO_SEC(p->stats.ticks_spent[PROC_STATUS_BLOCKED]);
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

        // 30% tolerance for lower limit
        tassert(secs >= (i * 70) / 100);
        tassert(secs <=  i);
    }
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

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    uint8_t secs = OSTICK_TO_SEC(p->stats.ticks_spent[PROC_STATUS_READY]);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    // 30% tolerance for lower limit
    tassert(secs >= (5 * 70) / 100);
    tassert(secs <=  5);
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

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    uint8_t secs = OSTICK_TO_SEC(p->stats.ticks_spent[PROC_STATUS_RUNNING]);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    // 30% tolerance for lower limit
    tassert(secs >= (5 * 70) / 100);
    tassert(secs <=  5);
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

static int chain(void *data) {
    uint8_t *nprocs = (uint8_t *) data;
    if (++(*nprocs) >= CONFIG_PROCESS_MAX_CONCURRENT_PROCS - 10) {
        return 0;
    }
    char buf[CONFIG_PROCESS_MAX_NAME_LEN] = { 0 };
    snprintf(buf, sizeof(buf), "proc%u", *nprocs);
    pcb_t *p = process_spawn_kernel_process(buf, chain, nprocs,
                        8196, process_get_current()->sched.priority);
    process_wait_for(p, NULL);
    return 0;
}

T(process_all_parents_wait_for_their_children) {
    uint8_t nprocs = 0;
    pcb_t *proc = process_spawn_kernel_process("proc0", chain, &nprocs,
                        8196,  process_get_current()->sched.priority);
    tassert(proc != NULL);
    process_wait_for(proc, NULL);
TEND

static bool end_orphan_test = false;
static int future_orphan(void *data) {
    uint8_t *nprocs = (uint8_t *) data;
    if (++(*nprocs) >= CONFIG_PROCESS_MAX_CONCURRENT_PROCS - 10) {
        sleep(5);
        end_orphan_test = true;
        return 0;
    }
    char buf[CONFIG_PROCESS_MAX_NAME_LEN] = { 0 };
    snprintf(buf, sizeof(buf), "proc%u", *nprocs);
    process_spawn_kernel_process(buf, future_orphan, nprocs,
                        8196, process_get_current()->sched.priority);
    msleep((CONFIG_PROCESS_MAX_CONCURRENT_PROCS - 10 - *nprocs) * 100);
    return 0;
}

T(process_orphan_children_are_adopted_by_grandparent) {
    uint8_t nprocs = 0;
    pcb_t *proc = process_spawn_kernel_process("proc0", future_orphan, &nprocs,
                        8196,  process_get_current()->sched.priority);
    tassert(proc != NULL);
    process_wait_for(proc, NULL);

    while (!end_orphan_test) {
        sleep(1);
    }
TEND
