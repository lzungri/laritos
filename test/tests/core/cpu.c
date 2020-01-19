#include <log.h>

#include <stdint.h>
#include <cpu.h>
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

T(cpu_cycle_count_properly_handles_bigger_than_32bits_values) {
    cpu_set_cycle_count_enable(true);
    uint64_t count;
    uint64_t countprev = 0;
    int i;
    // Counter should reach a > 32bits value before the 30 seconds mark
    for (i = 0; i < 30; i++) {
        sleep(1);
        count = cpu_get_cycle_count();
        // Counter should always increase, if not, then there was probably an overflow
        info("c=%llu", count);
        tassert(countprev < count);
        countprev = count;
    }
TEND
