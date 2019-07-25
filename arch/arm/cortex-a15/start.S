    .text
    .global _start
    .global _loop
    .type _start, %function
    .type _loop, %function

_start:
    mov r0, #1
    mov r1, #2
    add r2, r0, r1
    b _loop

_loop:
    b _loop
    # svc 0
