/* Memory layout extracted from qemu/hw/arm/virt.c */

MEMORY
{
    flash : ORIGIN = 0x00000000, LENGTH = 0x1000000000  /* 64 GB */
    ram :   ORIGIN = 0x40000000, LENGTH = 0x40000000    /* 1 GB */
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
        *(.text);
        . = ALIGN(4);
        __text_end = ABSOLUTE(.);
        __text_size = __text_end - __text_start;
    } > flash

    __data_lma = .;

    /* Data section/s will be loaded into the flash memory and then relocated manually in ram. */
    .data : {
        . = ALIGN(4);
        __data_start = ABSOLUTE(.);
        *(.data);
        BYTE(0xff)
        . = ALIGN(4);
        __data_end = ABSOLUTE(.);
        __data_size = __data_end - __data_start;
     } > ram AT > flash

     /* Non initialized data will be manually allocated (and zeroed) in ram */
    .bss : {
        . = ALIGN(4);
        __bss_start = ABSOLUTE(.);
        *(.bss)
        . = ALIGN(4);
        __bss_end = ABSOLUTE(.);
        __bss_size = __bss_end - __bss_start;
    } > ram

    /* Stack located at the end of the ram */
    . = ALIGN(4);
    __stack_top = ORIGIN(ram) + LENGTH(ram);
    /*__stack_end = __stacktop - STACK_SIZE */

    /* ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack") */
}