#include <display/gui.h>
#include <display/graphics.h>

void gui_init() {
    rect_t rect = { 0, 0, 100, 100 };
    draw_border_rect(rect, 0x00000000, 2, 0x00ffffff);
}
