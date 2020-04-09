#include <generated/autoconf.h>

/* Memory layout extracted from qemu/hw/arm/virt.c */

MEMORY
{
    flash : ORIGIN = CONFIG_SYS_FLASH_BASE, LENGTH = CONFIG_SYS_FLASH_SIZE
    ram :   ORIGIN = CONFIG_SYS_RAM_BASE, LENGTH = CONFIG_SYS_RAM_SIZE
}

SECTIONS
{
    /* Point the location counter to the origin of the flash memory */
    . = ORIGIN(flash);

    /* Code will be loaded into the flash memory */
    .text : {
        /* Vector table, located at address 0x0 */
        __vectors_start = ABSOLUTE(.);
        KEEP(*(.vectors))
        __vectors_end = ABSOLUTE(.);
        __vectors_size = __vectors_end - __vectors_start;

        /* Code */
        . = ALIGN(4);
        __text_start = ABSOLUTE(.);
        *(.text)

        /* Build id created by the linker */
        KEEP(*(.note.gnu.build-id))

        /* Keep the read only data in the flash */
        *(.rodata*)

        . = ALIGN(4);
        __modules_start = ABSOLUTE(.);
        KEEP(*(.modules))
        /* Mark the end of the modules array with a NULL pointer */
        LONG(0);

        . = ALIGN(4);
        __tests_start = ABSOLUTE(.);
        KEEP(*(.test))
        /* Mark the end of the tests array with a NULL pointer */
        LONG(0);

        /* ARM stuff */
        *(.glue*)
        *(.vfp11_veneer)
        *(.v4_bx*)

        . = ALIGN(4);
        __text_end = ABSOLUTE(.);
        __text_size = __text_end - __text_start;
    } > flash

    /* Data section/s will be loaded into the flash memory and then relocated manually in ram. */
    .data : {
        . = ALIGN(4);
        __data_start = ABSOLUTE(.);

        /* Map the board information at the beginning of the .data section */
        *boardinfo.o(*.data);
        /* End the board info with a null char, we handle it as a null-terminated string */
        BYTE(0)

        . = ALIGN(4);
        *(.data)
        . = ALIGN(4);
        __data_end = ABSOLUTE(.);
        __data_size = __data_end - __data_start;
     } > ram AT > flash

    /* Variable used to transfer the .data section from flash to ram */
    __data_lma = LOADADDR(.data);

    /* Kernel binary size (actual size of kernel.bin) */
    __kernel_bin_size = __data_lma + SIZEOF(.data);
    ASSERT(__kernel_bin_size < ORIGIN(flash) + LENGTH(flash), "Not enough Flash to fit the kernel")
    ASSERT(__kernel_bin_size < ORIGIN(flash) + CONFIG_FS_SYSTEM_IMAGE_BASE, "Kernel image overlaps with the system image")

    /* Non initialized data will be manually allocated (and zeroed) in ram */
    .bss : {
        . = ALIGN(4);
        __bss_start = ABSOLUTE(.);
        *(.bss)
        . = ALIGN(4);
        __bss_end = ABSOLUTE(.);
        __bss_size = __bss_end - __bss_start;
    } > ram
    ASSERT(__bss_end < ORIGIN(ram) + LENGTH(ram), "Not enough RAM to fit the OS .bss")

    /* Heap located right after the .bss section in the ram */
    . = ALIGN(4);
    __heap_start = .;
    __heap_end = __heap_start + CONFIG_MEM_HEAP_SIZE;
    . = __heap_end;
    ASSERT(__heap_end < ORIGIN(ram) + LENGTH(ram), "Not enough RAM to fit the OS heap")

    /* Stack located right after the heap (ram) */
    . = ALIGN(4);
    __stack_end = .;
    /* laritOS allocates stacks for the fiq, irq, undef, abort, svc processor modes
     + and a temporal stack for exception handlers (user mode stack will be allocated in the address
     * space of each process) */
    __stack_top = __stack_end + 6 * CONFIG_MEM_STACK_SIZE_PER_MODE;
    __stack_size = __stack_top - __stack_end;
    ASSERT(__stack_top < ORIGIN(ram) + LENGTH(ram), "Not enough RAM to fit the OS stacks")
}
