OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(boot_start)

SECTIONS
{
    . = 0x7c00;
    .boottext           : { *(.boottext) }
    .bootdata           : { *(.bootdata) }
    . = 0x7c00 + 510;
    .boottail           : { *(.boottail) }

    . = 0x7e00;
    .lotext             : { *(.lotext) }
    .lotext32           : { *(.lotext32) }

    . = ALIGN(32);
    .text               : { *(.text) }
    .text32             : { *(.text32) }
    .data               : { *(.data) }
    .rodata             : { *(.rodata) }
    .bss                : { *(.bss) }
}