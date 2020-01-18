#pragma once

static inline void arch_barrier_dmb(void) {
    asm("dmb");
}
