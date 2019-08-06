    .text
    .global _start
    .type _start, %function

_start:
    # Setup stack pointer (end of RAM for now)
    ldr sp, =__stack_top


    # Copy .data section from flash into ram
    ldr r0, =__data_lma
    ldr r1, =__data_start
    ldr r2, =__data_end

flash_to_ram_loop:
    cmp r1, r2
    # For optimization purposes, we copy 4-byte chunks instead of single bytes.
    # As a consequence of this, all addresses must be aligned to 4 bytes
    ldrlt r3, [r0], #4
    strlt r3, [r1], #4
    blt flash_to_ram_loop


    # Initialize (zero) bss section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0

zero_bss_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt zero_bss_loop


    # Use B instead of BL, kernel entry function doesn't return
    b kernel_entry
