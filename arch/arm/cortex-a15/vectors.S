    .text
    .global _vectors

_vectors:
    ldr pc, reset_addr
    ldr pc, undef_handler_addr
    ldr pc, svc_addr
    ldr pc, prefetch_addr
    ldr pc, abort_addr
    b . // Reserved vector
    ldr pc, irq_addr
    ldr pc, fiq_addr


    .align 4

reset_addr:         .word reset_handler
undef_handler_addr: .word undef_handler
svc_addr:           .word svc_handler
prefetch_addr:      .word prefetch_handler
abort_addr:         .word abort_handler
irq_addr:           .word irq_handler
fiq_addr:           .word fiq_handler

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

irq_handler:
    b irq_handler

fiq_handler:
    b fiq_handler
