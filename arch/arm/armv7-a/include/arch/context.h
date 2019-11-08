#pragma once

#include <cpu.h>
#include <string.h>
#include <process/pcb.h>

static inline void arch_context_usermode_init(struct pcb *pcb, void *lr) {
    /**
     * Stack layout:
     *
     *  sp (new) ->  spsr  <- PSR to be used on process startup
     *               lr    <- Entry point
     *               r0
     *               r1
     *               r2
     *               r3
     *               r4
     *               r5
     *               r6
     *               r7
     *               r8
     *               r9    <- GOT
     *               r10
     *               r11
     *               r12
     *               r13   --points to--,
     *  sp (old) ->  r14                |
     *               xxx   <------------'
     */
    uint32_t *sp = (uint32_t *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 4);

    // Clear all general-purpose regs
    memset(sp - 14, 0, sizeof(uint32_t) * 15);

    // Stack pointer at r13 (here is where the sp will be located right
    // after returning to user space)
    sp[-1] = (uint32_t) (sp + 1);

    // GOT at r9
    sp[-5] = (uint32_t) pcb->mm.got_start;
    sp -= 15;

    // LR (points to the executable entry point)
    *sp-- = (uint32_t) lr;

    regpsr_t psr = { 0 };
    // User mode
    psr.b.mode = 0b10000;
    // IRQ enabled (not masked)
    psr.b.irq = 0;
    // FIQ disabled
    psr.b.fiq = 1;
    // SPSR
    *sp = psr.v;

    // Setup process stack pointer
    pcb->mm.sp = sp;
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
