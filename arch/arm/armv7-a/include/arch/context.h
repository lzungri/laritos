#pragma once

#include <log.h>

#include <cpu.h>
#include <string.h>
#include <process/pcb.h>
#include <arch/context-types.h>

static inline bool arch_context_is_usr(spctx_t *ctx) {
    return ctx->spsr.b.mode == ARM_CPU_MODE_USER;
}

static inline bool arch_context_is_kernel(spctx_t *ctx) {
    return !arch_context_is_usr(ctx);
}

static inline void arch_context_init(struct pcb *pcb, void *retaddr, cpu_mode_t mode) {
    // Move back in the stack one spctx_t chunk
    pcb->mm.sp_ctx--;
    spctx_t *ctx = pcb->mm.sp_ctx;

    // Clear all regs
    memset(ctx, 0, sizeof(spctx_t));

    // Processor mode
    ctx->spsr.b.mode = mode == CPU_MODE_SUPERVISOR ? ARM_CPU_MODE_SUPERVISOR : ARM_CPU_MODE_USER;
    // IRQ enabled (not masked)
    ctx->spsr.b.irq = 0;
    // FIQ disabled
    ctx->spsr.b.fiq = 1;

    // LR (points to the executable entry point)
    ctx->ret = (int32_t) retaddr;

    // GOT at r9
    ctx->r[9] = (int32_t) pcb->mm.got_start;
}

static inline void arch_context_restore(pcb_t *pcb) {
    // The context restore will change based on whether we need to restore an svc
    // or user context. Check this by inspecting the psr value saved in the stack
    if (arch_context_is_kernel(pcb->mm.sp_ctx)) {
        regsp_t cursp = pcb->mm.sp_ctx;
        // Restore previous spctx (it was saved in r0 in arch_context_save_and_restore())
        pcb->mm.sp_ctx = (spctx_t *) pcb->mm.sp_ctx->r[0];

        // volatile to prevent any gcc optimization on the assembly code
        asm volatile (
            /* Restore stack pointer from pcb */
            "mov sp, %0            \n"
            /* Pop the SPSR and return address */
            "ldmfd sp!, {r0, lr}   \n"
            /* Update the SPSR (CPSR will be restored from this value)*/
            "msr spsr_cxsf, r0     \n"
            /* Restore all the other registers */
            "ldmfd sp!, {r0-r12}   \n"
            /* Save the return address in r0, (don't care if we lose r0 original value) */
            "mov r0, lr            \n"
            /* Get the original lr value from the saved context */
            "ldmfd sp!, {lr}       \n"
            /* Return to the address specified by the retaddr in the saved context.
             * subS also restores cpsr from spsr */
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
        "mov sp, %0               \n"
        /* Pop the SPSR and link register */
        "ldmfd sp!, {r0, lr}      \n"
        /* Update the SPSR (CPSR will be restored from this value)*/
        "msr spsr_cxsf, r0        \n"
        /* ^ to save the registers in the user bank (not the svc bank)*/
        "ldmfd sp!, {r0-r12, lr}^ \n"
        /* Trick to save the sp_svc into sp_user */
        "stmdb sp!, {sp}          \n"
        "ldmfd sp!, {sp}^         \n"
        /* subS to also restore cpsr from spsr */
        "subs pc, lr, #0          \n"
        :
        : "r" (pcb->mm.sp_ctx)
        : "memory", /* Memory barrier (do not reorder read/writes)*/
          "cc", /* Tell gcc this code modifies the cpsr */
          "r0" /* Do not use r0 in compiler generated code since we are using it */);
}

static inline void arch_context_save_and_restore(pcb_t *spcb, pcb_t *rpcb) {
    bool ctx_saved = false;
    // volatile to prevent any gcc optimization on the assembly code
    asm volatile (
        /* Save in r0 the previous spcb->mm.sp. We will use this value to restore the
         * previous pcb->mm.sp once this context is restored, see arch_context_restore()*/
        "mov r0, %1              \n"
        /* Push registers into sp_svc stack (remember that svc code uses the process stack) */
        "stmfd sp!, {r0-r12, lr} \n"
        /* Save current PSR */
        "mrs r0, cpsr_all        \n"
        /* Save return address in r1 (4 instructions ahead to set ctx_saved to false) */
        "add r1, pc, #8          \n"
        /* Save psr and retaddr in the stack */
        "stmfd sp!, {r0, r1}     \n"
        /* ctx_saved = true; */
        "mov %0, #1              \n"
        /* Skip the following instruction, that one will only be executed once the context is
         * restored */
        "b 1f                    \n"
        /* ctx_saved = true; */
        "mov %0, #0              \n"
     "1:                         \n"
        : "=&r" (ctx_saved)
        : "r" (spcb->mm.sp_ctx)
        : "memory",  /* Memory barrier (do not reorder read/writes) */
          "cc", /* Tell gcc this code modifies the cpsr */
          "r0", "r1" /* Do not use r0-r1 in compiler generated code since we are using them */);

    // Check whether this is a context save or returning from a context restore
    if (ctx_saved) {
        // Update the context pointer
        spcb->mm.sp_ctx = (spctx_t *) arch_regs_get_sp();

        // Since we screw the stack we need to do the saving and restoring here, that way,
        // when execution goes back to spcb, it will fix the stack and continue execution as
        // normal
        arch_context_restore(rpcb);

        // Execution will never reach this point
        // When spcb context is restored, it will continue right in the if (ctx_saved) instruction,
        // which will evaluate to false this time
    }
}
