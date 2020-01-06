#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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

    pcb_t *p1 = process_spawn_kernel_process("inc", atominc, &at,
                        8196, process_get_current()->sched.priority);
    tassert(p1 != NULL);
    pcb_t *p2 = process_spawn_kernel_process("inc2", atominc, &at,
                        8196, process_get_current()->sched.priority);
    tassert(p2 != NULL);
    pcb_t *p3 = process_spawn_kernel_process("dec", atomdec, &at,
                        8196, process_get_current()->sched.priority);
    tassert(p3 != NULL);

    process_wait_for(p1, NULL);
    process_wait_for(p2, NULL);
    process_wait_for(p3, NULL);

    tassert(atomic32_get(&at) == -10000000);
TEND
