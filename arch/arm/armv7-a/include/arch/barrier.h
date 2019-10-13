#pragma once

static inline void arch_dmb(void) {
    asm("dmb");
}
