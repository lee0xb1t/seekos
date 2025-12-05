OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_main)

KERNEL_PHYSICAL = 0;
KERNEL_VIRTUAL_OFFSET = 0xffffffff80000000;


PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = KERNEL_VIRTUAL_OFFSET;

    __kernel_start = .;
    

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));


    .text : 
    {
        __text_start = .;
        /* *(.stext) */
        *(.text) *(.text*)
        __text_end = .;
    }: text

    . = ALIGN(CONSTANT(MAXPAGESIZE));


    .rodata : 
    {
        __rodata_start = .;
        *(.rodata) *(.rodata*)
        __rodata_end = .;
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));


    .data : 
    {
        __data_start = .;
        *(.data) *(.data*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
        __data_end = .;
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));


    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}