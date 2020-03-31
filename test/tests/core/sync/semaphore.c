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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <test/test.h>
#include <sync/semaphore.h>
#include <test/utils/process.h>
#include <utils/utils.h>

T(semaphore_count_variable_is_updated_accordingly) {
    sem_t sem;
    sem_init(&sem, 1);
    tassert(sem.count == 1);
    sem_acquire(&sem);
    tassert(sem.count == 0);
    sem_release(&sem);
    tassert(sem.count == 1);
TEND

T(semaphore_count_variable_is_updated_accordingly2) {
    sem_t sem;
    sem_init(&sem, 5);
    tassert(sem.count == 5);
    sem_acquire(&sem);
    tassert(sem.count == 4);
    sem_acquire(&sem);
    tassert(sem.count == 3);
    sem_acquire(&sem);
    tassert(sem.count == 2);
    sem_acquire(&sem);
    tassert(sem.count == 1);
    sem_acquire(&sem);
    tassert(sem.count == 0);
    sem_release(&sem);
    tassert(sem.count == 1);
TEND

T(semaphore_count_variable_is_updated_accordingly3) {
    sem_t sem;
    sem_init(&sem, 0);
    tassert(sem.count == 0);
    sem_release(&sem);
    tassert(sem.count == 1);
    sem_acquire(&sem);
    tassert(sem.count == 0);
TEND


static int proc0(void *data) {
    sleep(5);
    sem_t *sem = (sem_t *) data;
    debug("Releasing semaphore");
    sem_release(sem);
    return 0;
}

T(semaphore_acquire_blocks_current_thread_if_sem_not_available) {
    sem_t sem;
    sem_init(&sem, 0);
    tassert(sem.count == 0);

    pcb_t *p1 = process_spawn_kernel_process("sema0", proc0, &sem,
                        8196, process_get_current()->sched.priority + 1);
    tassert(p1 != NULL);

    debug("Waiting for semaphore");
    sem_acquire(&sem);
    debug("Semaphore acquired");
    tassert(sem.count == 0);
    process_wait_for(p1, NULL);
TEND

static sem_t sems_012[4];

static int proc_012(void *data) {
    int i;
    for (i = 0; i < 2; i++) {
        int semidx = (int) data;
        sem_acquire(&sems_012[semidx]);
        debug("--- %u ---", semidx);
        sleep(1);
        sem_release(&sems_012[semidx + 1]);
    }
    return 0;
}

T(semaphore_012_sequence) {
    int i;
    for (i = 0; i < ARRAYSIZE(sems_012); i++) {
        sem_init(&sems_012[i], 0);
        tassert(sems_012[i].count == 0);
    }

    pcb_t *p0 = process_spawn_kernel_process("proc_0", proc_012, (void *) 0,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);
    pcb_t *p1 = process_spawn_kernel_process("proc_1", proc_012, (void *) 1,
                        8196, process_get_current()->sched.priority - 2);
    tassert(p1 != NULL);
    pcb_t *p2 = process_spawn_kernel_process("proc_2", proc_012, (void *) 2,
                        8196, process_get_current()->sched.priority - 3);
    tassert(p2 != NULL);

    schedule();

    irqctx_t pcbd_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(is_process_in(&p0->sched.sched_node, &sems_012[0].cond.blocked));
    tassert(is_process_in(&p1->sched.sched_node, &sems_012[1].cond.blocked));
    tassert(is_process_in(&p2->sched.sched_node, &sems_012[2].cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    sem_release(&sems_012[0]);
    sem_acquire(&sems_012[3]);

    sem_release(&sems_012[0]);
    sem_acquire(&sems_012[3]);
TEND

static int proc_acquire(void *data) {
    sem_t *sem = (sem_t *) data;
    sem_acquire(sem);
    debug("Semaphore acquired");
    return 0;
}

T(semaphore_blocked_proc_with_the_highest_priority_acquires_semaphore_after_it_is_released) {
    sem_t sem;
    sem_init(&sem, 0);

    pcb_t *p0 = process_spawn_kernel_process("proc0", proc_acquire, &sem,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);
    schedule();

    pcb_t *p1 = process_spawn_kernel_process("proc1", proc_acquire, &sem,
                        8196, process_get_current()->sched.priority - 2);
    tassert(p1 != NULL);
    schedule();

    pcb_t *p2 = process_spawn_kernel_process("proc2", proc_acquire, &sem,
                        8196, process_get_current()->sched.priority - 3);
    tassert(p2 != NULL);
    schedule();

    irqctx_t pcbd_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(is_process_in(&p0->sched.sched_node, &sem.cond.blocked));
    tassert(is_process_in(&p1->sched.sched_node, &sem.cond.blocked));
    tassert(is_process_in(&p2->sched.sched_node, &sem.cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    sem_release(&sem);
    sleep(1);
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p2->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p1->sched.status != PROC_STATUS_ZOMBIE);
    tassert(p0->sched.status != PROC_STATUS_ZOMBIE);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    sem_release(&sem);
    sleep(1);
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p0->sched.status != PROC_STATUS_ZOMBIE);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    sem_release(&sem);
    sleep(1);
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(p0->sched.status == PROC_STATUS_ZOMBIE);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
TEND
