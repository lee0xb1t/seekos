//
// https://wiki.osdev.org/Getting_VBE_Mode_Info
// https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial
//

// IT IS 16BIT ADDRESS!
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include "code16gcc.h"
#include "video.h"
#include "tty.h"


vesa_vbe_mode_info_t current_mode_info;

bool vesa_get_vesa_bios_information(vesa_vbe_info_t* vbe_info) {
    u16 ret = 0;

    u16 out_vbe_info;

    struct {
        u8 signature[4];
        u8 table_data[512-4];
    } __attribute__ ((packed)) input_vbe_info;

    for (int i = 0; i < 512; i++) {
        ((u8*)&input_vbe_info)[i] = 0;
    }

    input_vbe_info.signature[0] = 'V';
    input_vbe_info.signature[0] = 'B';
    input_vbe_info.signature[0] = 'E';
    input_vbe_info.signature[0] = '2';

    __asm__ __volatile__ (
        "movw   $0x4F00, %%ax;"
        "movw   %[in_info], %%di;"
        "int    $0x10        ;"
        "movw   %%ax, %[ret] ;"
        "movw   %%di, %[info] ;"
        : [ret] "=r" (ret), [info] "=r" (out_vbe_info)
        : [in_info] "r" ((u16)&input_vbe_info)
        : "ax", "di"
    );

    *vbe_info = *(vesa_vbe_info_t*)out_vbe_info;

    return ret == 0x004f;
}


bool vesa_mode_information(u16 mode, vesa_vbe_mode_info_t* vbe_mode_info) {
    u16 ret = 0;
    __asm__ __volatile__ (
        "movw   $0x4f01, %%ax;"
        "movw   %[mode], %%cx;"
        "movw   %[info], %%di;"
        "int    $0x10        ;"
        "movw   %%ax, %[ret] ;"
        : [ret] "=r" (ret)
        : [mode] "r" (mode), [info] "r" ((u16)vbe_mode_info)
        : "ax", "cx", "di"
    );
    return ret == 0x004f;
}


bool vesa_set_vbe_mode(u16 mode) {
    vesa_mode_t m;
    m.flag = mode;
    m.lfb = 1;

    u16 ret = 0;
    __asm__ __volatile__ (
        "movw   $0x4F02, %%ax;"
        "movw   %[mode], %%bx;"
        "int    $0x10        ;"
        "movw   %%ax, %[ret] ;"
        : [ret] "=r" (ret)
        : [mode] "r" (m)
        : "ax", "bx"
    );
    return ret == 0x004f;
}


bool vesa_get_current_mode(vesa_mode_t* current_mode) {
    u16 ret = 0;
    u16 mode = 0;
    __asm__ __volatile__ (
        "movw   $0x4F03, %%ax;"
        "int    $0x10        ;"
        "movw   %%ax, %[ret] ;"
        "movw   %%bx, %[mode] ;"
        : [ret] "=r" (ret), [mode] "=r" (mode)
        :
        : "ax"
    );
    current_mode->flag = mode;
    return ret == 0x004f;
}


bool video_find_best_vesa_mdoe(u16 *out_mode) {
    vesa_vbe_info_t vbe_info;
    bool success = vesa_get_vesa_bios_information(&vbe_info);

    if (!success) {
        return false;
    }

    u16* mdoes = (u16*)((vbe_info.video_modes_segment << 4) + vbe_info.video_modes_offset);

    u16 width = 1024, height = 768, bpp = 32;
    u16 best_mode = 0;

    u16 mode = 0;
    while ((mode = *mdoes++) != 0) {
        vesa_vbe_mode_info_t mode_info;
        vesa_mode_information(mode, &mode_info);
        
        if (mode_info.bpp >= bpp) {
            if (mode_info.width >= width && mode_info.height >= height) {
                width = mode_info.width;
                height = mode_info.height;
                best_mode = mode;
                break;
            }
        }
    }

    if (best_mode) {
        *out_mode = best_mode;
        return true;
    } else {
        return false;
    }
}


void video_init(vesa_vbe_mode_info_t** vbe_mode) {
    u16 bast_mode = 0;
    if (!video_find_best_vesa_mdoe(&bast_mode)) {
        puts("Cannot find best vesa mode\n");
        __asm__ __volatile__ ("hlt");
        for(;;){}
    }

    vesa_set_vbe_mode(bast_mode);

    if (!vesa_mode_information(bast_mode, &current_mode_info)) {
        puts("Get best vesa mode info faild\n");
        __asm__ __volatile__ ("hlt");
        for(;;){}
    }

    *vbe_mode = &current_mode_info;
}

vesa_vbe_mode_info_t* video_get_current_mode_info() {
    return &current_mode_info;
}