/* Memory layout extracted from qemu/hw/arm/virt.c */

MEMORY
{
    flash : ORIGIN = 0x00000000, LENGTH = 0x1000000000  /* 64 GB */
    ram :   ORIGIN = 0x40000000, LENGTH = 0x40000000 /* 1 GB */
}

SECTIONS
{
    .text : {
        *vectors.o(.text);
        *(.text);
    } > flash
}