#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <test/test.h>
#include <sync/rmutex.h>
#include <test/utils.h>
#include <utils/utils.h>
#include <process/core.h>

T(rmutex_init_initializes_mutex_as_expected) {
    rmutex_t m;
    rmutex_init(&m);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
TEND

T(rmutex_successful_acquire_sets_the_current_process_as_owner) {
    rmutex_t m;
    rmutex_init(&m);
    rmutex_acquire(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
TEND

T(rmutex_release_fails_if_mutex_is_not_locked) {
    rmutex_t m;
    rmutex_init(&m);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
    tassert(rmutex_release(&m) < 0);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
TEND

T(rmutex_process_will_not_block_on_acquire_if_already_took_the_lock) {
    rmutex_t m;
    rmutex_init(&m);
    rmutex_acquire(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
    rmutex_acquire(&m);
    tassert(m.lock_count == 2);
    tassert(m.owner == process_get_current());
    rmutex_acquire(&m);
    tassert(m.lock_count == 3);
    tassert(m.owner == process_get_current());
TEND

T(rmutex_release_must_match_num_of_acquire_to_successfully_unlock_mutex) {
    rmutex_t m;
    rmutex_init(&m);
    rmutex_acquire(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
    rmutex_acquire(&m);
    tassert(m.lock_count == 2);
    tassert(m.owner == process_get_current());
    rmutex_acquire(&m);
    tassert(m.lock_count == 3);
    tassert(m.owner == process_get_current());
    rmutex_release(&m);
    tassert(m.lock_count == 2);
    tassert(m.owner == process_get_current());
    rmutex_release(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
    rmutex_release(&m);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
TEND

static int mutex_owner(void *data) {
    rmutex_t *m = (rmutex_t *) data;
    rmutex_acquire(m);
    sleep(3);
    rmutex_release(m);
    return 0;
}

T(rmutex_only_owner_can_release_the_lock) {
    rmutex_t m;
    rmutex_init(&m);

    pcb_t *p1 = process_spawn_kernel_process("mowner", mutex_owner, &m,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p1 != NULL);

    schedule();

    tassert(m.lock_count == 1);
    tassert(m.owner == p1);
    tassert(rmutex_release(&m) < 0);
    tassert(m.lock_count == 1);
    tassert(m.owner == p1);

    process_wait_for(p1, NULL);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);

    rmutex_acquire(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
    rmutex_release(&m);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
TEND

static int mutex_owner2(void *data) {
    rmutex_t *m = (rmutex_t *) data;
    rmutex_acquire(m);
    sleep(3);
    rmutex_release(m);
    return 0;
}

T(rmutex_acquire_blocks_current_thread_if_mutex_is_not_available) {
    rmutex_t m;
    rmutex_init(&m);

    pcb_t *p1 = process_spawn_kernel_process("mowne2", mutex_owner2, &m,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p1 != NULL);

    schedule();

    tassert(m.lock_count == 1);
    tassert(m.owner == p1);

    rmutex_acquire(&m);
    tassert(m.lock_count == 1);
    tassert(m.owner == process_get_current());
    rmutex_release(&m);
    tassert(m.lock_count == 0);
    tassert(m.owner == NULL);
TEND


static int proc_acquire(void *data) {
    rmutex_t *m = (rmutex_t *) data;
    rmutex_acquire(m);
    debug("rmutex acquired");
    sleep(2);
    rmutex_release(m);
    return 0;
}

T(rmutex_blocked_proc_with_the_highest_priority_acquires_mutex_after_it_is_released) {
    rmutex_t m;
    rmutex_init(&m);
    rmutex_acquire(&m);

    pcb_t *p0 = process_spawn_kernel_process("proc0", proc_acquire, &m,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);
    schedule();

    pcb_t *p1 = process_spawn_kernel_process("proc1", proc_acquire, &m,
                        8196, process_get_current()->sched.priority - 2);
    tassert(p1 != NULL);
    schedule();

    pcb_t *p2 = process_spawn_kernel_process("proc2", proc_acquire, &m,
                        8196, process_get_current()->sched.priority - 3);
    tassert(p2 != NULL);
    schedule();

    irqctx_t pcbd_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    tassert(is_process_in(&p0->sched.sched_node, &m.cond.blocked));
    tassert(is_process_in(&p1->sched.sched_node, &m.cond.blocked));
    tassert(is_process_in(&p2->sched.sched_node, &m.cond.blocked));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    rmutex_release(&m);
    schedule();
    tassert(m.lock_count == 1);
    tassert(m.owner == p2);

    rmutex_acquire(&m);

    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);
    // Mutex can only be acquired by the test process if all the other procs
    // already took and released the mutex,
    tassert(p2->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p1->sched.status == PROC_STATUS_ZOMBIE);
    tassert(p0->sched.status == PROC_STATUS_ZOMBIE);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbd_ctx);

    rmutex_release(&m);
TEND
