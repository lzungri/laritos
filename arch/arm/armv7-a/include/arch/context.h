#pragma once

#include <log.h>

#include <cpu.h>
#include <string.h>
#include <process/core.h>
#include <arch/context-types.h>

static inline bool arch_context_is_usr(spctx_t *ctx) {
    return ctx->spsr.b.mode == ARM_CPU_MODE_USER;
}

static inline bool arch_context_is_kernel(spctx_t *ctx) {
    return !arch_context_is_usr(ctx);
}

static inline void arch_context_set_first_arg(spctx_t *ctx, void *arg) {
    // First function argument is passed via R0
    ctx->r[0] = (int32_t) arg;
}

static inline void arch_context_set_second_arg(spctx_t *ctx, void *arg) {
    // Second function argument is passed via R1
    ctx->r[1] = (int32_t) arg;
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
    // The context restore will change based on whether we need to restore a non-user
    // or user context. Check this by inspecting the psr value saved in the stack
    if (arch_context_is_kernel(pcb->mm.sp_ctx)) {
        regsp_t cursp = pcb->mm.sp_ctx;
        // Restore previous spctx
        pcb->mm.sp_ctx = pcb->mm.sp_ctx_prev;

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
            /* Save the return address in r2, (don't care if we lose r2 original value) */
            "mov r2, lr            \n"
            /* Get the original lr value from the saved context */
            "ldmfd sp!, {lr}       \n"
            /* Return to the address specified by the retaddr in the saved context.
             * subS also restores cpsr from spsr */
            "subs pc, r2, #0       \n"
            :
            : "r" (cursp)
            : "memory", /* Memory barrier (do not reorder read/writes)*/
              "cc", /* Tell gcc this code modifies the cpsr */
              "r0", "r2" /* Do not use r0, r2 in compiler generated code since we are using it */);

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
        "ldm sp, {r0-r12, lr}^    \n"
        "add sp, sp, #64          \n"
        /* Trick to save the sp_svc into sp_user
         * We could do something like:
         *   stmdb sp!, {sp}
         *   ldmfd sp!, {sp}^
         * But this will generate the warning "writeback of base register is UNPREDICTABLE"
         *
         * From ARM docs:
         *   Unpredictable instruction (forced user mode transfer with write-back to base)
         *   This is caused by an instruction such as PUSH {r0}^ where the ^ indicates
         *   access to user registers. The ARM Architectural Reference Manual specifies
         *   that writeback to the base register is not available with this instruction.
         **/
        "str sp, [sp, #-4]        \n"
        "sub sp, sp, #4           \n"
        "ldm sp, {sp}^            \n"
        "add sp, sp, #4           \n"
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
        /* Push registers into sp_svc/irq stack (remember that svc/irq code uses the process stack) */
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
        :
        : "memory",  /* Memory barrier (do not reorder read/writes) */
          "cc", /* Tell gcc this code modifies the cpsr */
          "r0", "r1", "r2" /* Do not use r0-r1 in compiler generated code since we are using them */
          /* Do not use r2 since we use it in arch_context_restore() as a temp buffer */);

    // Check whether this is a context save or returning from a context restore
    if (ctx_saved) {
        spcb->mm.sp_ctx_prev = spcb->mm.sp_ctx;
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
