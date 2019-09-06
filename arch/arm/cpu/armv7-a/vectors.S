    .section .vectors
    .global _vectors

_vectors:
    /* Occurs when the processor reset pin is asserted. This exception
     * is only expected to occur for signaling power-up, or for resetting
     * if the processor is already powered up. A soft reset can be done by
     * branching to the reset vector */
    ldr pc, =_reset_handler
    /* Occurs if neither the processor, nor any attached coprocessor, recognizes
     * the currently executing instruction */
    ldr pc, =_undef_handler
    /* This is a user-defined synchronous interrupt instruction. It enables
     * a program running in User mode, for example, to request privileged
     * operations that run in Supervisor mode, such as an RTOS function */
    ldr pc, =_svc_handler
    /* Occurs when the processor attempts to execute an instruction that was
     * not fetched, because the address was illegal */
    ldr pc, =_prefetch_handler
    /* Occurs when a data transfer instruction attempts to load or store data
     * at an illegal address */
    ldr pc, =_abort_handler
    ldr pc, =_reserved_handler
    /* Occurs when the processor external interrupt request pin is asserted
     * (LOW) and the I bit in the CPSR is clear */
    ldr pc, =_irq_handler
    /* Occurs when the processor external fast interrupt request pin is asserted
     * (LOW) and the F bit in the CPSR is clear */
    ldr pc, =_fiq_handler


    .align 4

_reset_handler:
    b _start

_undef_handler:
    b undef_handler

_svc_handler:
    # Save registers into the stack
    stmfd sp!, {r0-r12, lr}

    # Load the SVC instruction into r0 to get the service call number.
    # The instruction is located 4 bytes behind the lr
    ldr r0, [lr, #-4]
    # Mask the service call number (e.g. svc 1 -> 0xef000001) and save it in
    # r0 so that the svc_handler() gets it as the first argument
    bic r0, r0, #0xff000000
    # Set the stack pointer as the second argument for the handler
    mov r1, sp
    # Invoke the service call handler
    bl svc_handler

    # Pop the previously saved registers and continue execution in the svc
    # caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^

_prefetch_handler:
    b prefetch_handler

_abort_handler:
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

    bl abort_handler

    # Pop the previously saved registers and continue execution in the caller context.
    # ^ is used to restore the CPSR from the SPSR
    ldmfd sp!, {r0-r12, pc}^

_reserved_handler:
    b _reserved_handler

_irq_handler:
    b irq_handler

_fiq_handler:
    b fiq_handler
