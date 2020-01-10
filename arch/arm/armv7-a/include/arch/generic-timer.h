#pragma once

#include <log.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * The control register for the physical timer
 */
typedef struct {
    union {
        uint32_t v;
        struct {
            /**
             * Enables the timer. Permitted values are:
             *   0 Timer disabled.
             *   1 Timer enabled.
             * Setting this bit to 0 disables the timer output signal, but the timer value accessible from
             * CNTP_TVAL continues to count down
             */
            bool enable: 1;
            /**
             * Timer output signal mask bit. Permitted values are:
             *   0 Timer output signal is not masked.
             *   1 Timer output signal is masked.
             */
            bool imask: 1;
            /**
             * The status of the timer. This bit indicates whether the timer condition is asserted:
             *   0 Timer condition is not asserted.
             *   1 Timer condition is asserted.
             * When the ENABLE bit is set to 1, ISTATUS indicates whether the timer value meets the
             * condition for the timer output to be asserted
             */
            bool istatus: 1;
            uint32_t reserved: 29;
        } b;
    };
} regcntpctl_t;

static inline void arch_set_cntfrq(uint32_t freq) {
    asm("mcr p15, 0, %0, c14, c0, 0" : : "r" (freq));
}

static inline void arch_set_cntp_ctl(regcntpctl_t reg) {
    asm("mcr p15, 0, %0, c14, c2, 1" : : "r" (reg));
}

static inline void arch_set_cntp_cval(uint64_t val) {
    uint32_t cvall = (uint32_t) val;
    uint32_t cvalh = (uint32_t) (val >> 32);
    asm("mcrr p15, 2, %0, %1, c14" : : "r" (cvall), "r" (cvalh));
}

static inline uint64_t arch_get_cntp_ct(void) {
    uint32_t countl;
    uint32_t counth;
    asm("mrrc p15, 0, %0, %1, c14" : "=r" (countl), "=r" (counth));
    return (((uint64_t) counth) << 32) | countl;
}

static inline uint64_t arch_get_cntp_cval(void) {
    uint32_t countl;
    uint32_t counth;
    asm("mrrc p15, 0, %0, %1, c14" : "=r" (countl), "=r" (counth));
    return (((uint64_t) counth) << 32) | countl;
}
