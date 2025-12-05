ENTRY(_start)
SECTIONS
{
    . = 0x400000;

    .text :
    {
        *(.text)
        . = ALIGN(4096);
    }

    .data :
    {
        *(.data)
        *(.rodata)
        *(.bss)
        . = ALIGN(4096);
    }

    /DISCARD/ : 
    {
        *(.eh_frame)
        *(.note .note.*)
        *(.tls*)
    }
}
