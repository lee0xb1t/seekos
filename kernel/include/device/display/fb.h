#ifndef __fb_h__
#define __fb_h__

#include <bootparams.h>
#include <limine.h>


#define FB_BYTE_SIZE_PER_PIXEL      4

typedef union _bgrr_t {
    struct {
        u8 blue;
        u8 green;
        u8 red;
        u8 reserved;
    } u;
    u32 color;
} bgrr_t;

typedef union _rgbr_t {
    struct {
        u8 red;
        u8 green;
        u8 blue;
        u8 reserved;
    } u;
    u32 color;
} rgbr_t;

typedef enum _pixel_format {
    PIXEL_RGB = 0,
    PIXEL_BGR = 1,
    PIXEL_UNKNOWN,
} pixel_format;

typedef struct _fb_into_t {
    void *fb_ptr;
    uptr pfnn;
    u32 fb_size;
    u32 fb_nr_pages;
    u32 width;
    u32 height;
    pixel_format pixel_format;
    u32 actual_size;

    void *swap_ptr;
    u32 swap_size;
    u8 red_mask_shift;
    u8 green_mask_shift;
    u8 blue_mask_shift;
} fb_info_t;


void fb_init(struct limine_framebuffer_response *);

void fb_putpixel(u32 x, u32 y, u32 color);
void fb_refresh();
void fb_refresh_range(u32 start, u32 end);
void fb_test();
void fb_clear();
fb_info_t *fb_get_info();
void fb_scroll(int y);


#endif //__fb_h__
