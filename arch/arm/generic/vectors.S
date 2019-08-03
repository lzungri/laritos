    .section .vectors
    .global _vectors

_vectors:
    ldr pc, =reset_handler
    ldr pc, =undef_handler
    ldr pc, =svc_handler
    ldr pc, =prefetch_handler
    ldr pc, =abort_handler
    ldr pc, =reserved_handler
    ldr pc, =irq_handler
    ldr pc, =fiq_handler


    .align 4

# TODO Use weak functions

reset_handler:
    b _start

undef_handler:
    b undef_handler

svc_handler:
    b svc_handler

prefetch_handler:
    b prefetch_handler

abort_handler:
    b abort_handler

reserved_handler:
    b reserved_handler

irq_handler:
    b irq_handler

fiq_handler:
    b fiq_handler
