# Saves the current context in the stack.
# If the interrupted thread was running in:
#   - user mode: save user mode registers (see spctx_t) in the process stack
#   - non-user mode: save general registers and lr in the stack associated with
#                    the exception (e.g. sp_irq for irq exceptions)
#
# @param mode: Processor mode associated with the exception
# @return on r0: 1 on user mode context, 0 on non-user mode
.macro SAVE_CONTEXT mode
    # Load the exception temporal stack into sp
    ldr sp, =__stack_top
    # Save the current r0
    stmdb sp!, {r0}
    # Get the saved psr to check the cpu mode of the interrupted thread
    mrs r0, spsr
    # Mask cpu mode
    and r0, r0, #0b11111
    # user mode?
    cmp r0, #0b10000
    # Restore original r0
    ldmfd sp!, {r0}

    # \@ maintains a counter of how many macros it has executed in this pseudo-variable
    beq _save_ctx_usr_mode\@

_save_ctx_nonusr_mode\@:
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
    msr cpsr, \mode
    # Save interrupted thread lr into the stack
    str r2, [sp, #52]

    # Push spsr_<mode> and lr_<mode> registers into the interrupted thread stack
    stmdb sp!, {r0, lr}

    # Return value: non-user mode
    mov r0, #0
    b _save_ctx_end\@

_save_ctx_usr_mode\@:
    # To get the sp_user, ARM provides a priviledge mode called System that shares
    # the same register bank as the user mode but with more privileges
    # Switch to system mode (irq enabled / fiq disabled)
    msr cpsr_c, #0b11011111

    # Push user mode registers into the interrupted thread stack
    # See spctx_t for more info about stack layout
    sub sp, sp, #64
    stm sp, {r0-r12, lr}

    # Save sp_usr in r0 so that we can then assign it to sp_<mode> (r0 is shared among all modes)
    mov r0, sp

    # Switch back to the mode specified by the <mode> argument
    msr cpsr_c, \mode

    # Save sp_user in sp_<mode>
    mov sp, r0

    # Get the PSR and LR saved by the exception
    mrs r0, spsr
    mov r1, lr

    # Push spsr_<mode> and lr_<mode> registers into the interrupted thread stack
    sub sp, sp, #8
    stm sp, {r0, r1}^

    # Return value: user mode
    mov r0, #1

_save_ctx_end\@:

.endm


# Restores the current context from the stack associated with the process stack
.macro RESTORE_USER_CONTEXT
    # In case there was a context switch, get current sp context
    bl pcb_get_current_pcb_stack_context
    # Save sp in r1
    mov sp, r0

    # Pop the SPSR and link register
    ldm sp, {r0, lr}
    add sp, sp, #8

    # Update the SPSR (CPSR will be restored from this value)
    msr spsr_cxsf, r0
    # ^ to save the registers in the target processor mode (not the current bank)
    ldm sp, {r0-r12, lr}^
    add sp, sp, #64

    # Trick to save sp_<mode> into sp_user
    # We could do something like:
    #   stmdb sp!, {sp}
    #   ldmfd sp!, {sp}^
    # But this will generate the warning "writeback of base register is UNPREDICTABLE"
    #
    # From ARM docs:
    #   Unpredictable instruction (forced user mode transfer with write-back to base)
    #   This is caused by an instruction such as PUSH {r0}^ where the ^ indicates
    #   access to user registers. The ARM Architectural Reference Manual specifies
    #   that writeback to the base register is not available with this instruction.
    str sp, [sp, #-4]
    sub sp, sp, #4
    ldm sp, {sp}^
    add sp, sp, #4

    # subS to also restore cpsr from spsr
    subs pc, lr, #0
.endm


# Restores the current non-user context from the stack associated with
# the exception (e.g. sp_irq for irq exceptions)
.macro RESTORE_NONUSER_CONTEXT
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


# Restores the current non-user context from the stack associated with
# the exception (e.g. sp_irq for irq exceptions)
#
# @param handler: Handler symbol
# @param user_reg: Register in which the cpu mode is stored (1=user, 0=non-user)
.macro CALL_HANDLER_AND_RESTORE_CONTEXT handler, user_reg
    cmp \user_reg, #1
    beq _usr_mode\@

_nonusr_mode\@:
    bl \handler
    RESTORE_NONUSER_CONTEXT

_usr_mode\@:
    bl \handler
    RESTORE_USER_CONTEXT
.endm
