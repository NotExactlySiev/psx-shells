OUTPUT_FORMAT("binary")

EXTERN(_start)
ENTRY(_start)

TLOAD_ADDR = 0x80010000;

__ftext_len     = (__ftext_end - __ftext_start);
__fdata_len     = (__fdata_end - __fdata_start);


MEMORY
{
    loader      : ORIGIN = (TLOAD_ADDR - 0x800), LENGTH = 2048
    ram (rwx)   : ORIGIN = 0x80010000, LENGTH = 2M - (TLOAD_ADDR - 0x80000000)
}

SECTIONS
{
    .header : {
        
        BYTE(80); BYTE(83); BYTE(45); BYTE(88); 
        BYTE(32); BYTE(69); BYTE(88); BYTE(69);
        LONG(0); LONG(0);

        LONG(ABSOLUTE(_start)); /*entry*/
        LONG(0);
        LONG(TLOAD_ADDR);
        LONG(__ftext_len + __fdata_len);

        
        LONG(0); LONG(0);
        LONG(0); LONG(0);

        LONG(0x801FFF00); /* stack init */
        LONG(0);

        . = . + 1992;

    } > loader

     __ftext_start = ABSOLUTE(.);
    .text : { *(.text*) } > ram
    __ftext_end = ABSOLUTE(.);

    __fdata_start = ABSOLUTE(.);
    .rodata : { *(.rodata*) } > ram
    .data : { *(.data*) } > ram
    __fdata_end = ABSOLUTE(.);

    .sbss (NOLOAD) : {
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
    } > ram

    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) }

    /*. = ADDR(.text) - 0x800;*/
}
