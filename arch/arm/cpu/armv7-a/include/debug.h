#pragma once

#include <stdint.h>
#include <arm.h>

/**
 * @return: Processor mode string for the given <mode>
 */
static inline const char *get_cpu_mode_str(uint8_t mode) {
    static const char modes[16][4] = {
        "usr", "fiq", "irq", "svc", "???", "???", "mon", "abt",
        "???", "???", "hyp", "und", "???", "???", "???", "sys"
    };
    return modes[mode & 0xf];
}

void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, psr_t cpsr);
