    .section ".rodata"
    
    .global _terminal_font
    .align 8
    _terminal_font:
        .incbin "src/device/display/gohufont-14.psf"

    .global _terminal_font_bold
    .align 8
    _terminal_font_bold:
        .incbin "src/device/display/gohufont-14b.psf"

        

