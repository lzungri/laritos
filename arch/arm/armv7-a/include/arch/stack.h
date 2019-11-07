#pragma once

#include <stdint.h>
#include <arch/cpu.h>

/**
 * Structure mapping the set of registers pushed into the
 * stack by the handlers
 *
 * We need the following stack layout to be able to perform
 * context switches:
 *     sp (new) ->  spsr
 *                  lr
 *                  r0
 *                  r1
 *                  r2
 *                  r3
 *                  r4
 *                  r5
 *                  r6
 *                  r7
 *                  r8
 *                  r9
 *                  r10
 *                  r11
 *                  r12
 *     sp (old) ->  r13  ---points to--,
 *                  xxx <--------------'
 */
typedef struct {
    regpsr_t spsr;
    int32_t lr;
    int32_t r[13];
} spregs_t;
