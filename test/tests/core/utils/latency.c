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
