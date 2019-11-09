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
    spregs->lr = (int32_t) retaddr;

    // GOT at r9
    spregs->r[9] = (int32_t) pcb->mm.got_start;

    // Stack pointer at r13 (here is where the sp will be located right
    // after returning to user space)
    spregs->r[13] = (uint32_t) ((char *) spregs + sizeof(spregs_t));
}

/**
 *
 * @returns true if the function saved the context
 *          false if the function is returning from the restored context
 */
static inline bool arch_context_save(pcb_t *pcb) {
    bool ctx_saved = false;
    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        /* Switch to system mode (irq enabled / fiq disabled) */
        "msr cpsr_c, #0b01011111 \n"
        /* Push svc registers into the caller stack */
        "stmfd sp!, {r0-r14}    \n"
        /* Switch back to svc mode */
        "msr cpsr_c, #0b01010011 \n"
        /* Save current PSR */
        "mrs r0, cpsr_all        \n"
        /* Save return address in r1 (5 instructions ahead) */
        "add r1, pc, #12         \n"
        /* Switch to system mode (irq enabled / fiq disabled) */
        "msr cpsr_c, #0b01011111 \n"
        /* Save psr and retaddr in the stack */
        "stmfd sp!, {r0-r1}^     \n"
        /* Switch back to svc mode */
        "msr cpsr_c, #0b01010011 \n"
        /* ret = true; */
        "mov %0, #1              \n"
        : "=&r" (ctx_saved)
        :
        : "memory"  /* Memory barrier (do not reorder read/writes) */ ,
          "cc" /* Tell gcc this code modifies the cpsr */,
          "r0", "r1" /* Do not use r0 and r1 in compiler generated code since we are using them */);

    if (ctx_saved) {
        pcb->mm.sp = (regsp_t) ((char *) pcb->mm.sp - sizeof(spregs_t));
    }
    return ctx_saved;
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
