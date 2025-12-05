#include <device/display/fb.h>
#include <lib/kmemory.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <panic/panic.h>
#include <log/klog.h>


static volatile fb_info_t fb_info;
DECLARE_SPINLOCK(fb_lock);


void fb_init(struct limine_framebuffer_response *fb) {
    if (fb->framebuffer_count == 0) {
        panic("Not found frame buffer");
    }

    struct limine_framebuffer *lfb = fb->framebuffers[0];

    //fb->framebuffers[0]->address
    void *fb_virt = (void *)FRAME_BUFFER_START;
    fb_info.fb_ptr = fb_virt;
    fb_info.fb_size = lfb->height * lfb->pitch;
    fb_info.width = lfb->width;
    fb_info.height = lfb->height;
    fb_info.actual_size = PAGE_ALIGN_UP_IF(fb_info.fb_size);

    fb_info.red_mask_shift = lfb->red_mask_shift;
    fb_info.green_mask_shift = lfb->green_mask_shift;
    fb_info.blue_mask_shift = lfb->blue_mask_shift;

    uptr fb_phys = __VIRT_TO_PHYS(lfb->address);
    uptr fb_pfnn = (uptr)fb_phys >> PAGE_SHIFT;
    u32 nr_pages = SIZE_TO_PAGE(fb_info.actual_size);

    fb_info.pfnn = fb_pfnn;
    fb_info.fb_nr_pages = nr_pages;

    vmm_map(fb_virt, (uptr)fb_phys, nr_pages, VMM_FLAGS_MMIO);

    if (lfb->memory_model == LIMINE_FRAMEBUFFER_RGB) {
        fb_info.pixel_format = PIXEL_RGB;
    } else {
        fb_info.pixel_format = PIXEL_UNKNOWN;
    }

    void *swap_ptr = kzalloc(fb_info.actual_size);
    fb_info.swap_ptr = swap_ptr;
    fb_info.swap_size = fb_info.fb_size;

    memcpy(swap_ptr, fb_virt, fb_info.fb_size);
}


void fb_putpixel(u32 x, u32 y, u32 color) {
    
    u32 blue = (color >> fb_info.red_mask_shift) & 0xff;
    u32 green = (color >> fb_info.green_mask_shift) & 0xff;
    u32 red = (color >> fb_info.blue_mask_shift) & 0xff;

    /**
     * enum EFI_GRAPHICS_PIXEL_FORMAT {
     *     PixelRedGreenBlueReserved8BitPerColor,
     *     PixelBlueGreenRedReserved8BitPerColor,
     *     PixelBitMask,
     *     PixelBltOnly,
     *     PixelFormatMax
     * };
     */

    u32 color_;

    if (fb_info.pixel_format == PIXEL_RGB) {
        rgbr_t rgbr;
        rgbr.u.red = red;
        rgbr.u.green = green;
        rgbr.u.blue = blue;
        color_ = rgbr.color;
    } else if (fb_info.pixel_format == PIXEL_BGR) {
        bgrr_t bgrr;
        bgrr.u.red = red;
        bgrr.u.green = green;
        bgrr.u.blue = blue;
        color_ = bgrr.color;
    } else {
        panic("Unsupported pixel format");
    }

    u64 flags;
    spin_lock(&fb_lock);
    u32 *dest = ((u32 *)fb_info.swap_ptr) + x + (y * fb_info.width);
    *dest = color_;
    spin_unlock(&fb_lock);
}

void fb_refresh() {
    u64 flags;
    spin_lock(&fb_lock);
    memcpy(fb_info.fb_ptr, fb_info.swap_ptr, fb_info.fb_size);
    spin_unlock(&fb_lock);
}

void fb_refresh_range(u32 start, u32 end) {
    u64 flags;
    spin_lock(&fb_lock);

    if (start > end) {
        memset((u8 *)fb_info.swap_ptr + end * 4, 0, (start - end) * 4);
    } else {
        memcpy((u8 *)fb_info.fb_ptr + start * 4, (u8 *)fb_info.swap_ptr + start * 4, (end - start) * 4);
    }
    
    spin_unlock(&fb_lock);
}

void fb_test() {
    for (u32 i = 0; i < fb_info.width * (fb_info.height / 3); i++) {
        u32 x = i % fb_info.width;
        u32 y = (i - x) / fb_info.width;
        fb_putpixel(x, y, 0x00ff0000);
    }

    for (u32 i = fb_info.width * (fb_info.height / 3); i < fb_info.width * (fb_info.height / 3) * 2; i++) {
        u32 x = i % fb_info.width;
        u32 y = (i - x) / fb_info.width;
        fb_putpixel(x, y, 0x0000ff00);
    }

    for (u32 i = fb_info.width * (fb_info.height / 3) * 2; i < fb_info.width * (fb_info.height / 3) * 3; i++) {
        u32 x = i % fb_info.width;
        u32 y = (i - x) / fb_info.width;
        fb_putpixel(x, y, 0x000000ff);
    }
}

void fb_clear() {
    u64 flags;
    spin_lock(&fb_lock);
    memset(fb_info.swap_ptr, 0, fb_info.swap_size);
    spin_unlock(&fb_lock);
}

fb_info_t *fb_get_info() {
    return &fb_info;
}

void fb_scroll(int y) {
    u64 flags;
    spin_lock(&fb_lock);
    int skip = y * fb_info.width * FB_BYTE_SIZE_PER_PIXEL;
    int size = fb_info.swap_size - skip;
    memcpy(fb_info.swap_ptr, (u8 *)fb_info.swap_ptr + skip, size);
    memset((u8 *)fb_info.swap_ptr + size, 0, skip);
    spin_unlock(&fb_lock);
}
