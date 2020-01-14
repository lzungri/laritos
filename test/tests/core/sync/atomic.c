#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include <test/test.h>
#include <sync/atomic.h>
#include <test/utils.h>
#include <utils/utils.h>

T(atomic32_init_initializes_variable_accordingly) {
    atomic32_t at;
    atomic32_init(&at, 0);
    tassert(atomic32_get(&at) == 0);
    atomic32_init(&at, 1);
    tassert(atomic32_get(&at) == 1);
    atomic32_init(&at, -1);
    tassert(atomic32_get(&at) == -1);
TEND

T(atomic32_init_macro_initializes_variable_accordingly) {
    atomic32_t at = ATOMIC32_INIT(0);
    tassert(atomic32_get(&at) == 0);
    atomic32_t at2 = ATOMIC32_INIT(-1);
    tassert(atomic32_get(&at2) == -1);
    atomic32_t at3 = ATOMIC32_INIT(1);
    tassert(atomic32_get(&at3) == 1);
TEND

T(atomic32_add_increments_value_by_x) {
    atomic32_t at;
    atomic32_init(&at, 0);
    atomic32_add(&at, 3);
    tassert(atomic32_get(&at) == 3);
    tassert(atomic32_add(&at, 2) == 5);
    tassert(atomic32_add(&at, -5) == 0);
TEND

T(atomic32_overflows_when_value_equals_max32) {
    atomic32_t at;
    atomic32_init(&at, S32_MAX);
    tassert(atomic32_inc(&at) == S32_MIN);
TEND

T(atomic32_inc_increments_value_by_1) {
    atomic32_t at;
    atomic32_init(&at, 0);
    atomic32_inc(&at);
    tassert(atomic32_get(&at) == 1);
    tassert(atomic32_inc(&at) == 2);
TEND

T(atomic32_dec_decrements_value_by_1) {
    atomic32_t at;
    atomic32_init(&at, 1);
    tassert(atomic32_get(&at) == 1);
    atomic32_dec(&at);
    tassert(atomic32_get(&at) == 0);
    tassert(atomic32_dec(&at) == -1);
TEND

T(atomic32_set_stores_value_as_expected) {
    atomic32_t at = ATOMIC32_INIT(0);
    tassert(atomic32_get(&at) == 0);
    atomic32_set(&at, 1);
    tassert(atomic32_get(&at) == 1);
    atomic32_set(&at, 100);
    tassert(atomic32_get(&at) == 100);
    atomic32_set(&at, -100);
    tassert(atomic32_get(&at) == -100);
TEND

static int atominc(void *data) {
    atomic32_t *at = (atomic32_t *) data;
    int i;
    for (i = 0; i < 10000000; i++) {
        atomic32_inc(at);
    }
    return 0;
}

static int atomdec(void *data) {
    atomic32_t *at = (atomic32_t *) data;
    int i;
    for (i = 0; i < 30000000; i++) {
        atomic32_dec(at);
    }
    return 0;
}

T(atomic32_concurrent_changes_to_atomic_var_generate_correct_output) {
    atomic32_t at = ATOMIC32_INIT(0);
    tassert(atomic32_get(&at) == 0);

    pcb_t *p1 = process_spawn_kernel_process("inc", atominc, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p1 != NULL);
    pcb_t *p2 = process_spawn_kernel_process("inc2", atominc, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p2 != NULL);
    pcb_t *p3 = process_spawn_kernel_process("dec", atomdec, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p3 != NULL);

    process_wait_for(p1, NULL);
    process_wait_for(p2, NULL);
    process_wait_for(p3, NULL);

    tassert(atomic32_get(&at) == -10000000);
TEND


T(atomic64_init_initializes_variable_accordingly) {
    atomic64_t at;
    atomic64_init(&at, 0);
    tassert(atomic64_get(&at) == 0);
    atomic64_init(&at, 1);
    tassert(atomic64_get(&at) == 1);
    atomic64_init(&at, -1);
    tassert(atomic64_get(&at) == -1);
TEND

T(atomic64_init_macro_initializes_variable_accordingly) {
    atomic64_t at = ATOMIC64_INIT(0);
    tassert(atomic64_get(&at) == 0);
    atomic64_t at2 = ATOMIC64_INIT(-1);
    tassert(atomic64_get(&at2) == -1);
    atomic64_t at3 = ATOMIC64_INIT(1);
    tassert(atomic64_get(&at3) == 1);
TEND

T(atomic64_add_increments_value_by_x) {
    atomic64_t at;
    atomic64_init(&at, 0);
    atomic64_add(&at, 3);
    tassert(atomic64_get(&at) == 3);
    tassert(atomic64_add(&at, 2) == 5);
    tassert(atomic64_add(&at, -5) == 0);
TEND

T(atomic64_inc_increments_value_by_1) {
    atomic64_t at;
    atomic64_init(&at, 0);
    atomic64_inc(&at);
    tassert(atomic64_get(&at) == 1);
    tassert(atomic64_inc(&at) == 2);
TEND

T(atomic64_overflows_when_value_equals_max32) {
    atomic64_t at;
    atomic64_init(&at, S64_MAX);
    tassert(atomic64_inc(&at) == S64_MIN);
TEND

T(atomic64_inc_of_maxint32_gives_maxint32_plus_1) {
    atomic64_t at;
    atomic64_init(&at, S32_MAX);
    atomic64_inc(&at);
    tassert(atomic64_get(&at) == ((int64_t) S32_MAX) + 1);
    tassert(atomic64_inc(&at) == ((int64_t) S32_MAX) + 2);
TEND

T(atomic64_dec_decrements_value_by_1) {
    atomic64_t at;
    atomic64_init(&at, 1);
    tassert(atomic64_get(&at) == 1);
    atomic64_dec(&at);
    tassert(atomic64_get(&at) == 0);
    tassert(atomic64_dec(&at) == -1);
TEND

T(atomic64_set_stores_value_as_expected) {
    atomic64_t at = ATOMIC64_INIT(0);
    tassert(atomic64_get(&at) == 0);
    atomic64_set(&at, 1);
    tassert(atomic64_get(&at) == 1);
    atomic64_set(&at, 100);
    tassert(atomic64_get(&at) == 100);
    atomic64_set(&at, -100);
    tassert(atomic64_get(&at) == -100);
TEND

static int at64inc(void *data) {
    atomic64_t *at = (atomic64_t *) data;
    int i;
    for (i = 0; i < 10000000; i++) {
        atomic64_inc(at);
    }
    return 0;
}

static int at64dec(void *data) {
    atomic64_t *at = (atomic64_t *) data;
    int i;
    for (i = 0; i < 30000000; i++) {
        atomic64_dec(at);
    }
    return 0;
}

T(atomic64_concurrent_changes_to_atomic_var_generate_correct_output) {
    atomic64_t at = ATOMIC64_INIT(0);
    tassert(atomic64_get(&at) == 0);

    pcb_t *p1 = process_spawn_kernel_process("64inc", at64inc, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p1 != NULL);
    pcb_t *p2 = process_spawn_kernel_process("64inc2", at64inc, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p2 != NULL);
    pcb_t *p3 = process_spawn_kernel_process("64dec", at64dec, (void *) &at,
                        8196, process_get_current()->sched.priority);
    tassert(p3 != NULL);

    process_wait_for(p1, NULL);
    process_wait_for(p2, NULL);
    process_wait_for(p3, NULL);

    tassert(atomic64_get(&at) == -10000000);
TEND
