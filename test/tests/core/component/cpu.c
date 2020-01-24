#include <log.h>

#include <stdint.h>
#include <cpu/cpu.h>
#include <cpu/cpu-local.h>
#include <limits.h>
#include <test/test.h>
#include <time/time.h>
#include <utils/latency.h>

T(cpu_cycle_count_enable_disable_works_as_expected) {
    cpu_set_cycle_count_enable(false);
    uint64_t count = cpu_get_cycle_count();
    tassert(cpu_get_cycle_count() - count == 0);

    cpu_set_cycle_count_enable(true);
    int i;
    for (i = 0; i < 100; i++) {
        count = cpu_get_cycle_count();
        tassert(cpu_get_cycle_count() - count > 0);
    }
TEND

T(cpu_cycle_count_resets_when_disabled) {
    cpu_set_cycle_count_enable(true);
    uint64_t count = cpu_get_cycle_count();
    tassert(cpu_get_cycle_count() - count > 0);

    cpu_set_cycle_count_enable(false);
    tassert(cpu_get_cycle_count() == 0);
    cpu_set_cycle_count_enable(true);
    tassert(cpu_get_cycle_count() >= 0);
TEND

T(cpu_cycle_count_properly_handles_values_bigger_than_32bits) {
    cpu_set_cycle_count_enable(true);
    uint64_t count;
    uint64_t countprev = 0;
    int i;
    // Counter overflows every (U32_MAX / cpufreq) seconds, we wait that amount times 2
    for (i = 0; i < (U32_MAX / cpu()->freq) * 2; i++) {
        sleep(1);
        count = cpu_get_cycle_count();
        // Counter should always increase, if not, there was probably an unhandled overflow
        tassert(countprev < count);
        countprev = count;
    }
TEND

T(cpu_reset_cycle_count_works_as_expected) {
    cpu_set_cycle_count_enable(true);
    sleep(3);
    uint64_t count = cpu_get_cycle_count();
    cpu_reset_cycle_count();
    tassert(cpu_get_cycle_count() < count);
TEND
//
//T(cpu_cpu_local_defines_a_new_var_with_the_right_type) {
//    DEF_CPU_LOCAL(uint16_t, var);
//    tassert(sizeof(CPU_LOCAL_ATOMIC_GET_START(var)) == uint16_t);
//    CPU_LOCAL_ATOMIC_GET_END(var);
//
//    DEF_CPU_LOCAL(uint32_t, var2);
//    tassert(sizeof(CPU_LOCAL_ATOMIC_GET_START(var2)) == uint32_t);
//    CPU_LOCAL_ATOMIC_GET_END(var2);
//TEND

T(cpu_cpu_local_setter_and_getter_work_as_expected) {
    DEF_CPU_LOCAL(uint16_t, var);
    int i;
    for (i = 0; i < 100; i++) {
        CPU_LOCAL_SET(var, i);
        tassert(CPU_LOCAL_GET(var) == i);
    }
TEND

T(cpu_cpu_local_get_ptr_returns_the_cpu_local_reference) {
    DEF_CPU_LOCAL(uint16_t, var);
    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);
    tassert(CPU_LOCAL_GET_PTR_LOCKED(var) == &var[cpu_get_id()]);
    irq_local_restore_ctx(&ctx);
TEND
