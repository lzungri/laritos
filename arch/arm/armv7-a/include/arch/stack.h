#pragma once

#include <stdint.h>
#include <arch/cpu.h>

/**
 * Process context saved by the os in the process stack every time there
 * is a switch from user to kernel
 * This structure maps the set of registers pushed into the
 * stack by the handlers.
 *
 * We need the following stack layout to be able to perform
 * context switches:
 *     sp (new) ->  spsr (status register)
 *                  ret  (return address)
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
 *                  r11  (fp)
 *                  r12
 *     sp (old) ->  r14  (original lr value)
 */
typedef struct {
    regpsr_t spsr;
    int32_t ret;
    int32_t r[14];
} spctx_t;
