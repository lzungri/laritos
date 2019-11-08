#pragma once

#include <cpu.h>
#include <string.h>
#include <arch/stack.h>
#include <process/pcb.h>

static inline void arch_context_usermode_init(struct pcb *pcb, void *lr) {
    spregs_t *spregs = (spregs_t *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size);
    // Move back in the stack one spregs_t chunk
    spregs--;

    // Setup process stack pointer
    pcb->mm.sp = (regsp_t) spregs;

    // Clear all regs
    memset(spregs, 0, sizeof(spregs_t));

    // User mode
    spregs->spsr.b.mode = 0b10000;
    // IRQ enabled (not masked)
    spregs->spsr.b.irq = 0;
    // FIQ disabled
    spregs->spsr.b.fiq = 1;

    // LR (points to the executable entry point)
    spregs->lr = (int32_t) lr;

    // GOT at r9
    spregs->r[9] = (int32_t) pcb->mm.got_start;

    // Stack pointer at r13 (here is where the sp will be located right
    // after returning to user space)
    spregs->r[13] = (uint32_t) ((char *) spregs + sizeof(spregs_t));
}

static inline void arch_context_restore(pcb_t *pcb) {
    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        /* Restore stack pointer from pcb */
        "mov sp, %0            \n"
        /* Pop the SPSR and link register */
        "ldmfd sp!, {r0, lr}   \n"
        /* Update the SPSR (CPSR will be restored from this value)*/
        "msr spsr_cxsf, r0     \n"
        /* ^ to save the registers in the target processor mode (not the current bank)*/
        "ldmfd sp!, {r0-r14}^  \n"
        /* subS to also restore cpsr from spsr */
        "subs pc, lr, #0       \n"
        :
        : "r" (pcb->mm.sp)
        : "memory" /* Memory barrier (do not reorder read/writes)*/ ,
          "cc" /* Tell gcc this code modifies the cpsr */);
}
