#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <component/timer.h>

/**
 * PL031 Memory-mapped registers.
 *
 * Based on document: ARM PrimeCell Real Time Clock (PL031) Revision: r1p3
 */
typedef volatile struct {
    /**
     * RTCDR is a 32-bit read data register.
     * Reads from this register return the current value of the RTC
     */
    const uint32_t data;
    /**
     * RTCMR is a 32-bit read/write match register. Writes to this register load the match register, and
     * reads return the last written value. An equivalent match value is derived from this register. The
     * derived value is compared with the counter value in the CLK1HZ domain to generate an
     * interrupt
     */
    uint32_t match;
    /**
     * RTCLR is a 32-bit read/write load register. Writes to this register load an update value into the
     * RTC Update logic block where the updated value of the RTC is calculated. Reads return the last
     * written value
     */
    uint32_t load;
    /**
     * RTCCR is a 1-bit control register. When bit[0] == 1, the counter enable signal is asserted to
     * enable the counter
     * If set to 1, the RTC is enabled. After the RTC is enabled, do not write to this bit otherwise the current
     * RTC value is reset to zero.
     * A read returns the status of the RTC
     */
    uint32_t control;
    /**
     * RTCIMSC is a 1-bit read/write register, and controls the masking of the interrupt that the RTC
     * generates
     */
    uint32_t int_mask;
    /**
     * RTCRIS is read-only register. Reading this register gives the current raw status value of the
     * corresponding interrupt, prior to masking
     */
    const uint32_t raw_int_status;
    /**
     * RTCMIS is a 1-bit masked interrupt status register. It is a read-only register. Reading this
     * register gives the current masked status value of the corresponding interrupt
     */
    const uint32_t masked_int_status;
    /**
     * RTCICR is the interrupt clear register and is write only. Writing 1 to bit[0] clears the interrupt.
     * Writing 0 has no effect
     */
    uint32_t int_clear;
} pl031_mm_t;

typedef struct {
    timer_t parent;

    pl031_mm_t *mm;
} rtc_t;
