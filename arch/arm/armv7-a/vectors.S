    .section .vectors
    .global _vectors

/**
 * From ARM documentation:
 *
 * When an exception is generated, the processor performs the following actions:
 *   - Copies the CPSR into the appropriate SPSR. This saves the current mode, interrupt mask,
 *     and condition flags.
 *   - Switches state automatically if the current state does not match the instruction set
 *     used in the exception vector table.
 *   - Changes the appropriate CPSR mode bits to:
 *   - Change to the appropriate mode, and map in the appropriate banked out registers for
 *     that mode.
 *   - Disable interrupts. IRQs are disabled when any exception occurs. FIQs are disabled
 *     when an FIQ occurs and on reset.
 *   - Sets the appropriate LR to the return address.
 *   - Sets the PC to the vector address for the exception.
 */

_vectors:
    /* Occurs when the processor reset pin is asserted. This exception
     * is only expected to occur for signaling power-up, or for resetting
     * if the processor is already powered up. A soft reset can be done by
     * branching to the reset vector */
    ldr pc, =asm_reset_handler
    /* Occurs if neither the processor, nor any attached coprocessor, recognizes
     * the currently executing instruction */
    ldr pc, =asm_undef_handler
    /* This is a user-defined synchronous interrupt instruction. It enables
     * a program running in User mode, for example, to request privileged
     * operations that run in Supervisor mode, such as an RTOS function */
    ldr pc, =asm_svc_handler
    /* Occurs when the processor attempts to execute an instruction that was
     * not fetched, because the address was illegal */
    ldr pc, =asm_prefetch_handler
    /* Occurs when a data transfer instruction attempts to load or store data
     * at an illegal address */
    ldr pc, =asm_abort_handler
    ldr pc, =asm_reserved_handler
    /* Occurs when the processor external interrupt request pin is asserted
     * (LOW) and the I bit in the CPSR is clear */
    ldr pc, =asm_irq_handler
    /* Occurs when the processor external fast interrupt request pin is asserted
     * (LOW) and the F bit in the CPSR is clear */
    ldr pc, =asm_fiq_handler


    .align 4


asm_reset_handler:
    # Initialize system and start the kernel
    b _start


asm_undef_handler:
    # Save registers into the stack
    stmfd sp!, {r0-r12, lr}

    # First arg: Offending instruction
    sub r0, lr, #4
    # Second arg: Stack pointer
    mov r1, sp
    # Invoke the service call handler
    bl _undef_handler

    # Pop the previously saved registers and continue execution in the
    # caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^


asm_svc_handler:
    # First off, we need to get the user stack pointer and save it into sp_svc.
    # Note: SVC context uses the process stack

    # To get the sp_user, ARM provides a priviledge mode called System that shares
    # the same register bank as the user mode but with more privileges
    # Switch to system mode (irq enabled / fiq disabled)
    msr cpsr_c, #0b01011111

    # Push user mode registers (^) into the caller stack
    # See spregs_t for more info about stack layout
    stmfd sp!, {r0-r12, lr}^

    # Save sp_usr in r0 so that we can then assign it to sp_svc (r0 is shared among all modes)
    mov r0, sp

    # Switch back to svc mode
    msr cpsr_c, #0b01010011

    # Save sp_user in sp_svc
    mov sp, r0

    # Get the PSR and LR saved by the svc exception
    mrs r0, spsr
    mov r1, lr

    # Push spsr_svc and lr_svc registers into the caller stack
    stmdb sp!, {r0, r1}^

    # Load the SVC instruction into r0 to get the service call number.
    # The instruction is located 4 bytes behind the lr
    ldr r0, [r1, #-4]
    # Mask the service call number (e.g. svc 1 -> 0xef000001) and save it in
    # r0 so that the svc_handler() gets it as the first argument
    bic r0, r0, #0xff000000
    # Set the stack pointer as the second argument for the handler
    # We need this to update the pcb->mm.sp
    mov r1, sp

    # Invoke the service call handler
    bl _svc_handler

    # In case there was a context switch, get current sp context
    bl pcb_get_current_pcb_stack_context
    # Save sp in r1
    mov sp, r0

    # Pop the SPSR and link register
    ldmfd sp!, {r0, lr}
    # Update the SPSR (CPSR will be restored from this value)
    msr spsr_cxsf, r0
    # ^ to save the registers in the target processor mode (not the current svc bank)
    ldmfd sp!, {r0-r12, lr}^

    # Trick to save sp_svc into sp_user
    stmdb sp!, {sp}
    ldmfd sp!, {sp}^

    # subS to also restore cpsr from spsr
    subs pc, lr, #0


asm_prefetch_handler:
    # Point to the offending instruction, so that the cpu reexecutes the instruction
    # on return
    sub lr, lr, #4

    # Save registers into the stack
    stmfd sp!, {r0-r12, lr}

    # First arg: Offending instruction
    mov r0, lr
    # Second arg: Instruction fault status register (see ifsr_reg_t for more info)
    mrc p15, 0, r1, c5, c0, 1
    # Third arg: Stack pointer
    mov r2, sp

    bl _prefetch_handler

    # Pop the previously saved registers and continue execution in the caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^


asm_abort_handler:
    # According to ARM:
    # When a load or store instruction tries to access memory, the program
    # counter has been updated. A stored value of (pc – 4) in lr_ABT points
    # to the *second* instruction beyond the address where the exception was generated.

    # Point to the offending instruction, so that the cpu reexecutes the instruction
    # on return
    sub lr, lr, #8

    # Save registers into the stack
    stmfd sp!, {r0-r12, lr}

    # First arg: Offending instruction
    mov r0, lr
    # Second arg: Data fault status register (see dfsr_reg_t for more info)
    mrc p15, 0, r1, c5, c0, 0
    # Third arg: Stack pointer
    mov r2, sp

    bl _abort_handler

    # Pop the previously saved registers and continue execution in the caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^


asm_reserved_handler:
    b asm_reserved_handler


asm_irq_handler:
    # According to ARM:
    # After executing each instruction, the processor checks to see whether the interrupt
    # pins are LOW and whether the interrupt disable bits in the CPSR are clear. As a
    # result, IRQ or FIQ exceptions are generated only after the program counter has been
    # updated. Storing (pc – 4) in lr_mode causes lr_mode to point two instructions beyond
    # where the exception occurred. When the handler has finished, execution must continue
    # from the instruction prior to the one pointed to by lr_mode
    sub lr, lr, #4

    # Save registers into the stack
    stmfd sp!, {r0-r12, lr}

    # First arg: Stack pointer
    mov r0, sp
    bl _irq_handler

    # Pop the previously saved registers and continue execution in the caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^


asm_fiq_handler:
    b _fiq_handler
