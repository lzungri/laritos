#pragma once

#include <stdint.h>
#include <arm.h>

void dump_regs(const int32_t *regs, uint8_t nregs, int32_t pc, int32_t lr, psr_t cpsr);
