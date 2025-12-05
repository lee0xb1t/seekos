#include "loader.h"
#include "video.h"
#include <arch/cpuinstr.h>

#define PAGE_SIZE       (u32)4096
#define PAGE_SHIFT      12
#define PAGE_MASK	    (~(PAGE_SIZE-1))


#define PAGE_LEVEL1_ADDR 0x90000


void go_to_pm64_jmp(u32 kernel_start, void* kernel_params);


bool is_longmode_support() {
    __asm__ goto __volatile__ (
        "movl $0x80000000, %%eax ;"
        "cpuid                   ;"
        "xorl $0x80000000, %%eax ;"
        "jz %l[not_support]      ;"
        "movl $0x80000001, %%eax ;"
        "cpuid                   ;"
        "testl $(1 << 29), %%edx ;"
        "jz %l[not_support]      ;"
        :
        :
        : "eax", "edx", "cc"
        : not_support
);

    return true;

not_support:
    return false;
}

//
// ATA PIO mode
//
void ata_pio_rddisk(u32 sector, u32 sector_count, u8* buf) {
    outb(0x1f6, 0xe0);
    outb(0x1f2, (u8)(sector_count >> 8));
    outb(0x1f3, (u8)(sector >> 24));
    outb(0x1f4, 0);
    outb(0x1f5, 0);

    outb(0x1f2, (u8)(sector_count));
    outb(0x1f3, (u8)(sector));
    outb(0x1f4, (u8)(sector >> 8));
    outb(0x1f5, (u8)(sector >> 16));

    outb(0x1f7, 0x24);

    u16* data_buf = (u16*)buf;

    while (sector_count--) {
        while ((inb(0x1f7) & 0x88) != 0x8) {}

        for (int i = 0; i < SECTOR_SIZE / 2; i++) {
            *data_buf++ = inw(0x1f0);
        }
    }
}

void __draw_line(u32 color) {
    u32* framebuffer = boot_info.vbe_liner_frame_buffer;
    for (int y = 0; y < 16; y++) {
        framebuffer += boot_info.vbe_width;
        for (int x = 0; x < 16; x++) {
            framebuffer[x] = color;
        }
    }
    
}


void loader32_main(void) {
    if (!is_longmode_support()) {
        //puts("Don't support longmode.\n");
        __draw_line(0x00ff0000);  //red
        // halt();
        for (;;){}
    }
    
    // load and jmp to kernel
    // TOOD: ReadSize using build params
    ata_pio_rddisk(100, 512, (u8*)SYS_KERNEL_LOAD_ADDR);

    boot_info.kernel_start = SYS_KERNEL_LOAD_ADDR;

    boot_info.boot_virtual_memory_size = (u32)1024 * 1024 * 1024;
    boot_info.boot_pml4t_pa = PAGE_LEVEL1_ADDR;
    boot_info.boot_pml4t_self_mapping_index = 0x100;

    go_to_pm64_jmp(SYS_KERNEL_LOAD_ADDR, &boot_info);
    //((void(*)(const boot_info_t*))SYS_KERNEL_LOAD_ADDR)(&boot_info);
    for (;;) {}
}
