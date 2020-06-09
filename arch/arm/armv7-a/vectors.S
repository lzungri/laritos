#include "asm-macros.S"

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
 *   - Sets the appropriate LR_<excmode> to the return address.
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
    SAVE_CONTEXT #0b11011011

    # First arg: Offending instruction
    sub r0, lr, #4
    # Second arg: Stack pointer
    mov r1, sp

    CALL_HANDLER_AND_RESTORE_CONTEXT _undef_handler


asm_svc_handler:
    SAVE_CONTEXT #0b11010011

    # Load the SVC instruction into r0 to get the service call number.
    # The instruction is located 4 bytes behind the lr
    ldr r0, [lr, #-4]
    # Mask the service call number (e.g. svc 1 -> 0xef000001) and save it in
    # r0 so that the svc_handler() gets it as the first argument
    bic r0, r0, #0xff000000
    # Set the stack pointer as the second argument for the handler
    # We need this to update the pcb->mm.sp
    mov r1, sp

    CALL_HANDLER_AND_RESTORE_CONTEXT_WITH_RETVAL _svc_handler


asm_prefetch_handler:
    # Point to the offending instruction, so that the cpu reexecutes the instruction
    # on return
    sub lr, lr, #4

    SAVE_CONTEXT #0b11010111

    # First arg: Offending instruction
    mov r0, lr
    # Second arg: Instruction fault status register (see ifsr_reg_t for more info)
    mrc p15, 0, r1, c5, c0, 1
    # Third arg: Stack pointer
    mov r2, sp

    CALL_HANDLER_AND_RESTORE_CONTEXT _prefetch_handler


asm_abort_handler:
    # According to ARM:
    # When a load or store instruction tries to access memory, the program
    # counter has been updated. A stored value of (pc – 4) in lr_ABT points
    # to the *second* instruction beyond the address where the exception was generated.

    # Point to the offending instruction, so that the cpu reexecutes the instruction
    # on return
    sub lr, lr, #8

    SAVE_CONTEXT #0b11010111

    # First arg: Offending instruction
    mov r0, lr
    # Second arg: Data fault status register (see dfsr_reg_t for more info)
    mrc p15, 0, r1, c5, c0, 0
    # Third arg: Stack pointer
    mov r2, sp

    CALL_HANDLER_AND_RESTORE_CONTEXT _abort_handler


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

    SAVE_CONTEXT #0b11010010

    # First arg: Stack pointer
    mov r0, sp

    CALL_HANDLER_AND_RESTORE_CONTEXT _irq_handler


asm_fiq_handler:
    # laritOS doesn't support FIQs yet
    b _fiq_handler
