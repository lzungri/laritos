#include <generated/autoconf.h>

    .text
    .global _start

_start:

init_stacks:
    # Setup the OS stack pointers (fiq, irq, supervisor/svc, abort, undefined).
    # To configure the stack pointers, enter each mode with interrupts disabled,
    # and assign the appropriate value to the stack pointer.

    ldr r0, =__stack_top

    # irq/fiq disabled (0b110) | FIQ mode (0b10001)
    msr cpsr_c, #0b11010001
    mov sp, r0
    sub r0, #CONFIG_MEM_STACK_SIZE_PER_MODE

    # irq/fiq disabled (0b110) | IRQ mode (0b10010)
    msr cpsr_c, #0b11010010
    mov sp, r0
    sub r0, #CONFIG_MEM_STACK_SIZE_PER_MODE

    # irq/fiq disabled (0b110) | Abort mode (0b10111)
    msr cpsr_c, #0b11010111
    mov sp, r0
    sub r0, #CONFIG_MEM_STACK_SIZE_PER_MODE

    # irq/fiq disabled (0b110) | Undef mode (0b11011)
    msr cpsr_c, #0b11011011
    mov sp, r0
    sub r0, #CONFIG_MEM_STACK_SIZE_PER_MODE

    # irq/fiq disabled (0b110) | SVC mode (0b10011)
    msr cpsr_c, #0b11010011
    mov sp, r0

    # Keep the processor in supervisor mode


copy_data_section:
    # Copy OS .data section from flash into ram
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



init_bss_section:
    # Initialize (zero) OS bss section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0

zero_bss_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt zero_bss_loop



start_kernel:
    # Use B instead of BL, kernel entry function doesn't return
    b kernel_entry
