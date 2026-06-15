#include <device/display/fb.h>
#include <base/spinlock.h>
#include <lib/kmemory.h>
#include <lib/kmath.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <panic/panic.h>
#include <log/klog.h>


static fb_info_t fb_info;
DECLARE_SPINLOCK(fb_lock);


static u32 _pack_color(u32 red, u32 green, u32 blue) {
    switch (fb_info.pixel_format) {
    case PIXEL_RGB: {
        rgb_t rgb;
        rgb.u.red   = (u8)blue;
        rgb.u.green = (u8)green;
        rgb.u.blue  = (u8)red;
        rgb.u.reserved = 0;
        return rgb.color;
    }
    case PIXEL_BGR: {
        bgr_t bgr;
        bgr.u.blue  = (u8)red;
        bgr.u.green = (u8)green;
        bgr.u.red   = (u8)blue;
        bgr.u.reserved = 0;
        return bgr.color;
    }
    default:
        return 0;
    }
}


void fb_init(struct limine_framebuffer_response *fb) {
    if (fb->framebuffer_count == 0) {
        panic("Not found frame buffer");
    }

    struct limine_framebuffer *lfb = fb->framebuffers[0];

    u32 pitch = lfb->pitch;
    u32 fb_sz = lfb->height * pitch;

    fb_info.fb_size       = fb_sz;
    fb_info.width         = lfb->width;
    fb_info.height        = lfb->height;
    fb_info.pitch         = pitch;
    fb_info.actual_size   = PAGE_ALIGN_UP_IF(fb_sz);

    fb_info.red_mask_shift   = lfb->red_mask_shift;
    fb_info.green_mask_shift = lfb->green_mask_shift;
    fb_info.blue_mask_shift  = lfb->blue_mask_shift;

    uptr fb_phys     = __VIRT_TO_PHYS((uptr)lfb->address);
    uptr fb_pfnn     = fb_phys >> PAGE_SHIFT;
    u32  nr_pages    = SIZE_TO_PAGE(fb_info.actual_size);
    fb_info.pfnn       = fb_pfnn;
    fb_info.fb_nr_pages = nr_pages;

    void *fb_virt = (void *)FRAME_BUFFER_START;
    fb_info.fb_ptr = fb_virt;

    vmm_map(fb_virt, fb_phys, nr_pages, VMM_FLAGS_MMIO);

    if (lfb->memory_model == LIMINE_FRAMEBUFFER_RGB) {
        fb_info.pixel_format = PIXEL_RGB;
    } else {
        fb_info.pixel_format = PIXEL_UNKNOWN;
    }

    void *swap_ptr = kzalloc(fb_info.actual_size);
    fb_info.swap_ptr = swap_ptr;
    fb_info.swap_size = fb_sz;

    memcpy(swap_ptr, fb_virt, fb_sz);

    klogi("Framebuffer: %ux%u, pitch=%u, format=%s\n",
        fb_info.width, fb_info.height, pitch,
        fb_info.pixel_format == PIXEL_RGB ? "RGB" : "unknown");
}


void fb_putpixel(u32 x, u32 y, u32 color) {
    if (x >= fb_info.width || y >= fb_info.height)
        return;

    u32 packed = _pack_color(
        (color >> 16) & 0xff,   /* red   */
        (color >> 8)  & 0xff,   /* green */
        color         & 0xff);  /* blue  */

    u64 flags;
    spin_lock_irq(&fb_lock, flags);
    u32 *dest = ((u32 *)fb_info.swap_ptr) + y * fb_info.width + x;
    *dest = packed;
    spin_unlock_irq(&fb_lock, flags);
}


void fb_flush(void) {
    u64 flags;
    spin_lock_irq(&fb_lock, flags);
    memcpy(fb_info.fb_ptr, fb_info.swap_ptr, fb_info.fb_size);
    spin_unlock_irq(&fb_lock, flags);
}


void fb_flush_region(u32 byte_start, u32 byte_end) {
    u64 flags;

    if (byte_start >= fb_info.fb_size || byte_end > fb_info.fb_size)
        return;
    if (byte_start >= byte_end)
        return;

    u32 len = byte_end - byte_start;

    spin_lock_irq(&fb_lock, flags);
    memcpy((u8 *)fb_info.fb_ptr + byte_start,
           (u8 *)fb_info.swap_ptr + byte_start, len);
    spin_unlock_irq(&fb_lock, flags);
}


void fb_clear(void) {
    u64 flags;
    spin_lock_irq(&fb_lock, flags);
    memset(fb_info.swap_ptr, 0, fb_info.swap_size);
    spin_unlock_irq(&fb_lock, flags);
}


void fb_clear_region(u32 byte_start, u32 byte_end) {
    u64 flags;

    if (byte_start >= fb_info.fb_size || byte_end > fb_info.fb_size)
        return;
    if (byte_start >= byte_end)
        return;

    u32 len = byte_end - byte_start;

    spin_lock_irq(&fb_lock, flags);
    memset((u8 *)fb_info.swap_ptr + byte_start, 0, len);
    spin_unlock_irq(&fb_lock, flags);
}


void fb_scroll(u32 pixel_rows) {
    u64 flags;

    u32 skip_bytes = pixel_rows * fb_info.width * FB_BYTE_SIZE_PER_PIXEL;
    if (skip_bytes == 0 || skip_bytes >= fb_info.swap_size)
        return;

    u32 keep_bytes = fb_info.swap_size - skip_bytes;

    spin_lock_irq(&fb_lock, flags);

    u8 *base = (u8 *)fb_info.swap_ptr;
    for (u32 ofs = 0; ofs < keep_bytes; ofs++) {
        base[ofs] = base[ofs + skip_bytes];
    }

    memset(base + keep_bytes, 0, skip_bytes);

    spin_unlock_irq(&fb_lock, flags);
}


void fb_test(void) {
    u32 w = fb_info.width;
    u32 h = fb_info.height;

    for (u32 i = 0; i < w * (h / 3); i++)
        fb_putpixel(i % w, i / w, 0x00ff0000);

    for (u32 i = w * (h / 3); i < w * (h / 3) * 2; i++)
        fb_putpixel(i % w, i / w, 0x0000ff00);

    for (u32 i = w * (h / 3) * 2; i < w * (h / 3) * 3; i++)
        fb_putpixel(i % w, i / w, 0x000000ff);
}


fb_info_t *fb_get_info(void) {
    return &fb_info;
}

u32 fb_get_width(void) {
    return fb_info.width;
}

u32 fb_get_height(void) {
    return fb_info.height;
}


void fb_putpixel_nolock(u32 x, u32 y, u32 color) {
    if (x >= fb_info.width || y >= fb_info.height)
        return;

    u32 packed = _pack_color(
        (color >> 16) & 0xff,
        (color >> 8)  & 0xff,
        color         & 0xff);

    u32 *dest = ((u32 *)fb_info.swap_ptr) + y * fb_info.width + x;
    *dest = packed;
}

void fb_putpixel_direct(u32 x, u32 y, u32 color) {
    if (x >= fb_info.width || y >= fb_info.height)
        return;

    u32 packed = _pack_color(
        (color >> 16) & 0xff,
        (color >> 8)  & 0xff,
        color         & 0xff);

    u32 *dest = ((u32 *)fb_info.fb_ptr) + y * fb_info.width + x;
    *dest = packed;
}

void fb_flush_nolock(void) {
    if (!fb_info.fb_ptr || !fb_info.swap_ptr)
        return;
    memcpy(fb_info.fb_ptr, fb_info.swap_ptr, fb_info.fb_size);
}

void fb_fill_nolock(u32 x, u32 y, u32 w, u32 h, u32 color) {
    if (x >= fb_info.width || y >= fb_info.height)
        return;
    u32 x2 = x + w;
    u32 y2 = y + h;
    if (x2 > fb_info.width)  x2 = fb_info.width;
    if (y2 > fb_info.height) y2 = fb_info.height;

    u32 packed = _pack_color(
        (color >> 16) & 0xff,
        (color >> 8)  & 0xff,
        color         & 0xff);

    u32 *base = (u32 *)fb_info.swap_ptr;
    for (u32 row = y; row < y2; row++) {
        u32 *row_ptr = base + row * fb_info.width + x;
        for (u32 col = x; col < x2; col++) {
            *row_ptr++ = packed;
        }
    }
}

void fb_fill_direct(u32 x, u32 y, u32 w, u32 h, u32 color) {
    if (x >= fb_info.width || y >= fb_info.height)
        return;
    u32 x2 = x + w;
    u32 y2 = y + h;
    if (x2 > fb_info.width)  x2 = fb_info.width;
    if (y2 > fb_info.height) y2 = fb_info.height;

    u32 packed = _pack_color(
        (color >> 16) & 0xff,
        (color >> 8)  & 0xff,
        color         & 0xff);

    u32 *base = (u32 *)fb_info.fb_ptr;
    for (u32 row = y; row < y2; row++) {
        u32 *row_ptr = base + row * fb_info.width + x;
        for (u32 col = x; col < x2; col++) {
            *row_ptr++ = packed;
        }
    }
}

void fb_scroll_nolock(u32 pixel_rows) {
    u32 skip_bytes = pixel_rows * fb_info.width * FB_BYTE_SIZE_PER_PIXEL;
    if (skip_bytes == 0 || skip_bytes >= fb_info.swap_size)
        return;

    u32 keep_bytes = fb_info.swap_size - skip_bytes;

    u8 *base = (u8 *)fb_info.swap_ptr;
    for (u32 ofs = 0; ofs < keep_bytes; ofs++) {
        base[ofs] = base[ofs + skip_bytes];
    }
    memset(base + keep_bytes, 0, skip_bytes);
}
