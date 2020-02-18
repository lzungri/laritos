#include <log.h>

#include <stdint.h>
#include <cpu/core.h>
#include <test/test.h>
#include <time/core.h>
#include <utils/latency.h>

T(latency_latencynoirq_disable_irqs_as_expected) {
    LATENCY_NOIRQ("no irq", {
        tassert(!irq_is_enabled());
    });
TEND

T(latency_logs_latency_info_1us_sleep) {
    int i;
    for (i = 0; i < 100; i++) {
        LATENCY("usleep(1)", {
            usleep(1);
        });
    }
TEND

T(latency_logs_latency_info_1ms_sleep) {
    int i;
    for (i = 0; i < 100; i++) {
        LATENCY("msleep(1)", {
            msleep(1);
        });
    }
TEND

T(latency_logs_latency_info_1sec_sleep) {
    int i;
    for (i = 0; i < 10; i++) {
        LATENCY("sleep(1)", {
            sleep(1);
        });
    }
TEND

T(latency_logs_latency_info_5sec_sleep) {
    int i;
    for (i = 0; i < 3; i++) {
        LATENCY("sleep(5)", {
            sleep(5);
        });
    }
TEND
