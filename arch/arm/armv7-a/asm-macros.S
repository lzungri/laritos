# Saves the current context in the stack.
# If the interrupted thread was running in:
#   - user mode: save user mode registers (see spctx_t) in the process stack
#   - non-user mode: save general registers and lr in the stack associated with
#                    the exception (e.g. sp_irq for irq exceptions)
#
# @param mode: Processor mode associated with the exception
.macro SAVE_CONTEXT mode
_save_ctx\@:
    # TODO Fix this once reentrant exceptions are implemented (disable interrupts
    # in this section, maybe?)
    # Load the exception temporal stack into sp
    ldr sp, =__stack_top
    # Save the current r0
    stmdb sp!, {r0}
    ldr r0, =_laritos
    ldr r0, [r0]
    cmp r0, #1
    # Restore original r0
    ldmfd sp!, {r0}
    beq _save_ctx_process_mode\@

_save_ctx_nonprocess_mode\@:
    # Save registers into the exception stack
    # lr is just a placeholder here, it will be replaced with the lr associated with
    # the interrupted thread
    stmfd sp!, {r0-r12, lr}

    # Get the PSR saved by the exception
    mrs r0, spsr

    # Disable irq, otherwise when we switch modes, it will call the irq handler again
    orr r1, r0, #0b10000000

    # Switch to mode running before the irq occured
    msr cpsr, r1
    # Save the lr associated with the interrupted thread into r2
    mov r2, lr

    # Switch back to the mode specified by the <mode> argument
    msr cpsr_c, \mode

    # Save interrupted thread lr into the stack
    str r2, [sp, #52]

    # Push spsr_<mode> and lr_<mode> registers into the interrupted thread stack
    stmdb sp!, {r0, lr}

    b _save_ctx_end\@

_save_ctx_process_mode\@:
    # TODO Fix this once reentrant exceptions are implemented (disable interrupts
    # in this section, maybe?)
    # Save the current r0, r1
    stmdb sp!, {r0, r1}
    # Get the saved psr to check the cpu mode of the interrupted thread
    mrs r0, spsr

    # Disable irq, otherwise when we switch modes, it will call the irq handler again
    orr r1, r0, #0b10000000

    # Mask cpu mode
    and r0, r0, #0b11111

    # user mode?
    cmp r0, #0b10000
    bne _switch_and_save_ctx\@
    mov r1, #0b11011111

_switch_and_save_ctx\@:
    msr cpsr, r1

    stmfd sp!, {r2-r12, lr}

    mov r2, sp

    msr cpsr_c, \mode

    # Restore original r0
    ldmfd sp!, {r0, r1}

    mov sp, r2
    stmfd sp!, {r0, r1}

    mrs r0, spsr
    stmdb sp!, {r0, lr}

_save_ctx_end\@:

.endm


# Restores the current non process context from the stack associated with
# the exception (e.g. sp_irq for irq exceptions)
.macro RESTORE_NONPROCESS_CONTEXT
    # Pop the PSR and lr
    ldmfd sp!, {r0, lr}

    # Update the SPSR (CPSR will be restored from this value)
    msr spsr_cxsf, r0

    # Restore some registers, the rest will be used as temp regs
    ldmfd sp!, {r0-r9}

    # Save current psr
    mrs r10, cpsr
    # Save previous psr
    mrs r11, spsr
    # Disable irq, otherwise when we switch modes, it will call the irq handler again
    orr r11, r11, #0b10000000
    # Save irq_sp
    mov r12, sp

    # Switch to mode running before the irq occured
    msr cpsr_c, r11
    # Load its lr from the sp_irq
    ldr lr, [r12, #12]

    # Switch back to irq mode
    msr cpsr_c, r10
    # Restore all the remaining regs
    ldmfd sp!, {r10-r12}
    # lr was already read, so update sp to point to the end of the frame
    add sp, sp, #4

    # subS to also restore cpsr from spsr
    subs pc, lr, #0
.endm


# Calls the <handler> and restores the context on return
#
# @param handler: Handler symbol
.macro CALL_HANDLER_AND_RESTORE_CONTEXT handler
_restore_ctx\@:
    ldr r4, =_laritos
    ldr r4, [r4]
    cmp r4, #1
    beq _restore_ctx_process_mode\@

_restore_ctx_nonprocess_mode\@:
    bl \handler
    RESTORE_NONPROCESS_CONTEXT

_restore_ctx_process_mode\@:
    bl \handler
    # Get current process and restore its context
    bl asm_process_get_current
    # Current process is passed via r0
    b asm_arch_context_restore
.endm

# Calls the <handler> and restores the context on return, it also saves the return
# value of the handler in r0 for the caller to access it
#
# @param handler: Handler symbol
.macro CALL_HANDLER_AND_RESTORE_CONTEXT_WITH_RETVAL handler
_restore_ctx\@:
    ldr r4, =_laritos
    ldr r4, [r4]
    cmp r4, #1
    beq _restore_ctx_process_mode\@

_restore_ctx_nonprocess_mode\@:
    bl \handler
    RESTORE_NONPROCESS_CONTEXT

_restore_ctx_process_mode\@:
    bl \handler
    # Save retval in r4
    mov r4, r0
    # Get current process stack context in r0
    bl asm_process_get_current_pcb_stack_context
    # Update context with the return value (retval is sent via r0)
    str r4, [r0, #8]
    # Get current process and restore its context
    bl asm_process_get_current
    # Current process is passed via r0
    b asm_arch_context_restore
.endm
