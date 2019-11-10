#pragma once

#include <log.h>

#include <cpu.h>
#include <string.h>
#include <arch/stack.h>
#include <process/pcb.h>

static inline void arch_context_init(struct pcb *pcb, void *retaddr, cpu_mode_t mode) {
    spregs_t *spregs = (spregs_t *) pcb->mm.sp;
    // Move back in the stack one spregs_t chunk
    spregs--;

    // Setup process stack pointer
    pcb->mm.sp = (regsp_t) spregs;

    // Clear all regs
    memset(spregs, 0, sizeof(spregs_t));

    // Processor mode
    spregs->spsr.b.mode = mode == CPU_MODE_SUPERVISOR ? ARM_CPU_MODE_SUPERVISOR : ARM_CPU_MODE_USER;
    // IRQ enabled (not masked)
    spregs->spsr.b.irq = 0;
    // FIQ disabled
    spregs->spsr.b.fiq = 1;

    // LR (points to the executable entry point)
    spregs->ret = (int32_t) retaddr;

    // GOT at r9
    spregs->r[9] = (int32_t) pcb->mm.got_start;
}

static inline void arch_context_restore(pcb_t *pcb) {
    regpsr_t *psr = (regpsr_t *) pcb->mm.sp;

    if (psr->b.mode == ARM_CPU_MODE_SUPERVISOR) {
        regsp_t cursp = pcb->mm.sp;
        // Restore previous sp
        pcb->mm.sp = (regsp_t) ((spregs_t *) pcb->mm.sp)->r[0];

        // volatile to prevent any gcc optimization on the assembly code
        asm volatile (
            /* Restore stack pointer from pcb */
            "mov sp, %0            \n"
            /* Pop the SPSR and link register */
            "ldmfd sp!, {r0, lr}   \n"
            /* Update the SPSR (CPSR will be restored from this value)*/
            "msr spsr_cxsf, r0     \n"
            "ldmfd sp!, {r0-r12}   \n"
            /* Don't care if we lose r0 original value */
            "mov r0, lr            \n"
            "ldmfd sp!, {lr}   \n"
            /* subS to also restore cpsr from spsr */
            "subs pc, r0, #0       \n"
            :
            : "r" (cursp)
            : "memory", /* Memory barrier (do not reorder read/writes)*/
              "cc", /* Tell gcc this code modifies the cpsr */
              "r0" /* Do not use r0 in compiler generated code since we are using it */);

        return;
    }

    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        /* Restore stack pointer from pcb */
        "mov sp, %0            \n"
        /* Pop the SPSR and link register */
        "ldmfd sp!, {r0, lr}   \n"
        /* Update the SPSR (CPSR will be restored from this value)*/
        "msr spsr_cxsf, r0     \n"
        /* ^ to save the registers in the user bank (not the svc bank)*/
        "ldmfd sp!, {r0-r12, lr}^ \n"
        "stmdb sp!, {sp}   \n"
        "ldmfd sp!, {sp}^  \n"
        /* subS to also restore cpsr from spsr */
        "subs pc, lr, #0       \n"
        :
        : "r" (pcb->mm.sp)
        : "memory", /* Memory barrier (do not reorder read/writes)*/
          "cc", /* Tell gcc this code modifies the cpsr */
          "r0" /* Do not use r0 in compiler generated code since we are using it */);
}

static inline void arch_context_save_and_restore(pcb_t *spcb, pcb_t *rpcb) {
    bool ctx_saved = false;
    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        /* Save in r0 the previous spcb->mm.sp */
        "mov r0, %1              \n"
        "stmfd sp!, {r0-r12, lr} \n"
        /* Save current PSR */
        "mrs r0, cpsr_all        \n"
        /* Save return address in r1 (4 instructions ahead to set ctx_saved to false) */
        "add r1, pc, #8          \n"
        /* Save psr and retaddr in the stack */
        "stmfd sp!, {r0, r1}     \n"
        /* ctx_saved = true; */
        "mov %0, #1              \n"
        "b 1f                    \n"
        "mov %0, #0              \n"
     "1:                         \n"
        : "=&r" (ctx_saved)
        : "r" (spcb->mm.sp)
        : "memory",  /* Memory barrier (do not reorder read/writes) */
          "cc", /* Tell gcc this code modifies the cpsr */
          "r0", "r1", "r2" /* Do not use r0-r2 in compiler generated code since we are using them */);

    if (ctx_saved) {
        spcb->mm.sp = arch_regs_get_sp();

        // Since we screw the stack we need to do the saving and restoring here, that way,
        // when execution goes back to spcb, it will fix the stack and continue execution as
        // normal
        arch_context_restore(rpcb);

        // Execution will never reach this point
        // When spcb context is restored, it will continue right in the if (ctx_saved) instruction,
        // which will evaluate to false this time
    }
}
