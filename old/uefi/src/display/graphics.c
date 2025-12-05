#include <display/graphics.h>
#include <stdint.h>

extern EFI_SYSTEM_TABLE *gST;

EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;


void graph_init() {
    gST->BootServices->LocateProtocol(&gop_guid, NULL, (void**)&gop);
}

EFI_GRAPHICS_OUTPUT_PROTOCOL* graph_get_mode() {
    return gop;
}


void draw_pixel(int x, int y, uint32_t rrgb) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;
    uint32_t hr = gop->Mode->Info->HorizontalResolution;

    int reversed = (rrgb >> 24) & 0xff;
    int red = (rrgb >> 16) & 0xff;
    int green = (rrgb >> 8) & 0xff;
    int blue = (rrgb >> 0) & 0xff;

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dest = fb + x + (y * hr);
    dest->Reserved = reversed;
    dest->Red = red;
    dest->Green = green;
    dest->Blue = blue;
}

void draw_rect(rect_t rect, uint32_t color) {
    int x = rect.x;
    int y = rect.y;
    int width = rect.width;
    int height = rect.height;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}


void draw_border_rect(rect_t rect, uint32_t color, int border_width, uint32_t border_color) {
    int x = rect.x;
    int y = rect.y;
    int width = rect.width;
    int height = rect.height;

    int content_x = x + border_width;
    int content_y = y + border_width;
    int content_width = width - border_width;
    int content_height = height - border_width;

    for (int row = 0; row < width; row++) {
        for (int line = 0; line < height; line++) {
            // draw_pixel(content_x + j, content_y + i, color);
            if ((line < content_x && row < content_y)                       // top
                || (line >= content_x && row < content_y)

                || (line > content_width - 1 && row > content_height - 1)   // right
                || (line > content_width - 1 && row <= content_height - 1)

                || (line < content_x && row > content_height - 1)           // bottom
                || (line >= content_x && row > content_height - 1)

                || (line < content_x && row - 1 < content_y)                // left
                || (line < content_x && row - 1 >= content_y)) {

                draw_pixel(line, row, border_color);
            } else {
                draw_pixel(line, row, color);
            }
        }
    }
}
