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
    b svc_handler

_prefetch_handler:
    b prefetch_handler

_abort_handler:
    b abort_handler

_reserved_handler:
    b _reserved_handler

_irq_handler:
    b irq_handler

_fiq_handler:
    b fiq_handler
