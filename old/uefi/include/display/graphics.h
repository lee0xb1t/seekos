#ifndef __graphics_h__
#define __graphics_h__

#include <efi.h>

typedef struct {
    int x;
    int y;
    int width;
    int height;
} rect_t;

void graph_init();

EFI_GRAPHICS_OUTPUT_PROTOCOL* graph_get_mode();

void draw_rect(rect_t rect, uint32_t color);

void draw_border_rect(rect_t rect, uint32_t color, int border_width, uint32_t border_color);

#endif //__graphics_h__