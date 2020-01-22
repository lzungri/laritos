#pragma once

#include <log.h>

#include <cpu/cpu.h>
#include <string.h>
#include <process/core.h>
#include <arch/debug.h>
#include <arch/context-types.h>

static inline bool arch_context_is_usr(spctx_t *ctx) {
    return ctx->spsr.b.mode == ARM_CPU_MODE_USER;
}

static inline bool arch_context_is_kernel(spctx_t *ctx) {
    return !arch_context_is_usr(ctx);
}

static inline void *arch_context_get_retaddr(spctx_t *ctx) {
    return (void *) ctx->ret;
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

static inline void arch_context_restore(spctx_t *spctx) {
    regpsr_t curpsr = arch_cpu_get_cpsr();
    regpsr_t targetpsr = { 0 };
    // The context restore will change based on whether we need to restore a non-user
    // or user context. Check this by inspecting the psr value saved in the stack
    if (arch_context_is_kernel(spctx)) {
        targetpsr = spctx->spsr;
        // Disable IRQs
        targetpsr.b.irq = 1;
        // Disable FIQs (not supported yet)
        targetpsr.b.fiq = 1;
    } else {
        // If user mode, then switch to System mode, which will give us access to all the
        // user registers plus the ability to switch back to curpsr
        targetpsr.b.mode = ARM_CPU_MODE_SYSTEM;
        // Disable IRQs
        targetpsr.b.irq = 1;
        // Disable FIQs (not supported yet)
        targetpsr.b.fiq = 1;
    }

    // Check if the target and current processor mode are the same. If they are, then the switch
    // logic is simpler since we don't have to switch modes back and forth
    if (curpsr.b.mode == targetpsr.b.mode) {
        // volatile to prevent any gcc optimization on the assembly code
        asm volatile (
            /* Copy cursp into r0*/
            "mov r0, %0              \n"
            /* Load target mode and ret address (lr will be overwritten later) */
            "ldmfd r0!, {r1, lr}     \n"
            /* Save cursp + 8 into sp */
            "mov sp, r0              \n"
            /* Set current mode (to update condition flags, etc)*/
            "msr cpsr, r1            \n"
            /* Restore registers */
            "ldmfd sp!, {r0-r12, lr} \n"
            /* Set pc equals ret address */
            "ldr pc, [sp, #-60]      \n"
            :
            : "r" (spctx)
            : "memory", /* Memory barrier (do not reorder read/writes)*/
              "cc", /* Tell gcc this code modifies the cpsr */
              "r0", "r1" /* Do not use r0, r2 in compiler generated code since we are using it */);
    } else {
        // volatile to prevent any gcc optimization on the assembly code
        asm volatile (
            /* Copy cursp into r0*/
            "mov r0, %0              \n"
            /* Load target mode and ret address */
            "ldmfd r0!, {r1, lr}     \n"
            /* Update saved PSR, CPSR wlll be set to this value with the SUBS instr later */
            "msr spsr_cxsf, r1       \n"
            /* Save target psr into r1 */
            "mov r1, %1              \n"
            /* Change to target mode */
            "msr cpsr, r1            \n"
            /* Save curpsr into r1. These kind of instructions will use other auxiliary
             * registers (e.g. r12) and therefore we need to execute them before
             * restoring r0-r12 */
            "mov r1, %2              \n"
            /* Skip r0 and r1 registers (those are going to be restored later) */
            "add sp, r0, #8          \n"
            /* Restore registers (r0, r1, will be used as temporal regs and restored later)*/
            "ldmfd sp!, {r2-r12, lr} \n"
            /* Switch back to cursp */
            "msr cpsr_c, r1          \n"
            /* Point SP to the beginning of the rX registers */
            "mov sp, r0              \n"
            /* Restore r0 and r1 */
            "ldmfd sp!, {r0, r1}     \n"
            /* Jump and switch modes */
            "subs pc, lr, #0         \n"
            :
            : "r" (spctx), "r" (targetpsr.v), "r" (curpsr.v)
            : "memory", /* Memory barrier (do not reorder read/writes)*/
              "cc", /* Tell gcc this code modifies the cpsr */
              "r0", "r1" /* Do not use r0, r2 in compiler generated code since we are using it */);
    }
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
        // Update the context pointer
        spcb->mm.sp_ctx = (spctx_t *) arch_cpu_get_sp();

        // Since we screw the stack we need to do the saving and restoring here, that way,
        // when execution goes back to spcb, it will fix the stack and continue execution as
        // normal
        arch_context_restore(rpcb->mm.sp_ctx);

        // Execution will never reach this point
        // When spcb context is restored, it will continue right in the if (ctx_saved) instruction,
        // which will evaluate to false this time
    }
}
