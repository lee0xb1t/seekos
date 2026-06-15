#ifndef __fb_h__
#define __fb_h__

#include <bootparams.h>
#include <limine.h>


#define FB_BYTE_SIZE_PER_PIXEL      4

typedef union _bgr_t {
    struct {
        u8 blue;
        u8 green;
        u8 red;
        u8 reserved;
    } u;
    u32 color;
} bgr_t;

typedef union _rgb_t {
    struct {
        u8 red;
        u8 green;
        u8 blue;
        u8 reserved;
    } u;
    u32 color;
} rgb_t;

typedef enum _pixel_format {
    PIXEL_RGB = 0,
    PIXEL_BGR = 1,
    PIXEL_UNKNOWN,
} pixel_format;

typedef struct _fb_info_t {
    void    *fb_ptr;
    uptr     pfnn;
    u32      fb_size;
    u32      fb_nr_pages;
    u32      width;
    u32      height;
    u32      pitch;
    pixel_format pixel_format;
    u32      actual_size;

    void    *swap_ptr;
    u32      swap_size;

    u8       red_mask_shift;
    u8       green_mask_shift;
    u8       blue_mask_shift;
} fb_info_t;


void fb_init(struct limine_framebuffer_response *);

void fb_putpixel(u32 x, u32 y, u32 color);

void fb_flush(void);
void fb_flush_region(u32 byte_start, u32 byte_end);

void fb_clear(void);
void fb_clear_region(u32 byte_start, u32 byte_end);

void fb_scroll(u32 pixel_rows);
void fb_test(void);

fb_info_t *fb_get_info(void);
u32 fb_get_width(void);
u32 fb_get_height(void);

void fb_putpixel_nolock(u32 x, u32 y, u32 color);
void fb_putpixel_direct(u32 x, u32 y, u32 color);
void fb_flush_nolock(void);
void fb_fill_nolock(u32 x, u32 y, u32 w, u32 h, u32 color);
void fb_fill_direct(u32 x, u32 y, u32 w, u32 h, u32 color);
void fb_scroll_nolock(u32 pixel_rows);


#endif
