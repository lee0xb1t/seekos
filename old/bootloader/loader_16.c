#include "code16gcc.h"
#include "loader.h"
#include "pm.h"
#include "video.h"
#include "tty.h"

#include <arch/cpuinstr.h>


#define VIDEO_MDOE_1024_768_24      0x0118      // 0x0118  1024x768    24/32*          16m


//
// kernel
//
boot_info_t boot_info;


//
// pm
//
void loader32_entry(void);
void go_to_pm32_jmp(void);

// limit_low | base_low | base_middle + accsess_byte | limit_high + flags + base_high
u16 loader_gdt_entries[][4] = {
    { 0, 0, 0, 0 },    // empty segment
    { 0xffff, 0x0000, 0x9a00, 0x00cf },    // kernel code segment    | 0x8
    { 0xffff, 0x0000, 0x9200, 0x00cf },    // kernel data segment    | 0x10

    { 0xffff, 0x0000, 0x9a00, 0x00af },    // kernel code64 segment  | 0x18
    { 0xffff, 0x0000, 0x9200, 0x00af },    // kernel data64 segment  | 0x20
};

static inline void open_fast_a20() {
    __asm__ __volatile__ (
        "inb $0x92, %al;"
        "or $2, %al    ;"
        "outb %al, $0x92;"
    );
}

static inline void goto_protected_mode() {
    open_fast_a20();
    _cli();
    // TODO: disable NMI
    // TODO: mask all interrupts using PIC
    _lgdt(loader_gdt_entries, 5);
    go_to_pm32_jmp();
    // no return
}


void loader_main(void) {
    vesa_vbe_mode_info_t*  vbe_mode_info = null;
    video_init(&vbe_mode_info);

    puts("Video initial success!\n");
    puts("Loader main...\n");

    boot_info.vbe_liner_frame_buffer = (void*)vbe_mode_info->framebuffer;
    boot_info.vbe_width = vbe_mode_info->width;
    boot_info.vbe_height = vbe_mode_info->height;
    boot_info.vbe_bpp = vbe_mode_info->bpp;
    boot_info.vbe_pitch = vbe_mode_info->pitch;

    detect_memory();

    goto_protected_mode();
    // no return
}
