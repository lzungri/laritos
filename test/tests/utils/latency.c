#include <log.h>

#include <stdint.h>
#include <cpu.h>
#include <test/test.h>
#include <time/time.h>
#include <utils/latency.h>

T(latency_latencynoirq_disable_irqs_as_expected) {
    LATENCY_NOIRQ("no irq", {
        tassert(!irq_is_enabled());
    });
TEND

T(latency_logs_latency_info) {
    int i;
    for (i = 0; i < 5; i++) {
        LATENCY("sleep(1)", {
            sleep(1);
        });
    }
TEND
