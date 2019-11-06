#pragma once

#include <cpu.h>
#include <process/pcb.h>


static inline void arch_restore_context(pcb_t *pcb) {
    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        "mov sp, %0               \n"
        "ldmfd sp!, {r0, lr}      \n"
        "msr spsr_all, r0         \n"
        "ldmfd sp!, {r0-r12}      \n"
        "subs pc, lr, #0          \n"
        :
        : "r" (pcb->mm.sp)
        : "memory" /* Memory barrier (do not reorder read/writes)*/ , "cc" /* Tell gcc this code modifies the cpsr */);
}
