#include <device/display/term.h>
#include <device/display/psf_font.h>
#include <device/display/fb.h>
#include <lib/kcolor.h>
#include <base/spinlock.h>


extern psf1_font_t _terminal_font, _terminal_font_bold;
static term_info_t g_term_info;

DECLARE_SPINLOCK(term_lock);


void term_init(term_render_t render) {
    fb_info_t *fb_info = fb_get_info();
    
    g_term_info.font_width = PSF1_FONT_WIDTH;
    g_term_info.font_height = 14;
    g_term_info.ch_width = fb_info->width / g_term_info.font_width;
    g_term_info.ch_height = fb_info->height / g_term_info.font_height;
    g_term_info.bgcolor = COLOR_BLACK;
    g_term_info.fgcolor = COLOR_WHITE;
    g_term_info.bold = false;
    g_term_info.render = render;
}

void term_putch(u32 color, char ch) {
    u64 flags;
    spin_lock(&term_lock);

    if (ch == '\n') {
        g_term_info.ch_y++;
        g_term_info.ch_x = 0;
    }

    if (ch == '\t') {
        // g_term_info.ch_y++;
        // g_term_info.ch_x = 4 - (g_term_info.ch_x % 4);
        // if (g_term_info.ch_x >= g_term_info.ch_width) {
        // }

        int space = 8 - (g_term_info.ch_x % 8);
        for (int i = 0; i < space; i++) {
            spin_unlock(&term_lock);
            term_putch(color, ' ');
            spin_lock(&term_lock);
        }

        spin_unlock(&term_lock);
        return;
    }

    if (g_term_info.ch_x >= g_term_info.ch_width) {
        g_term_info.ch_x = 0;
        g_term_info.ch_y++;
    }

    if (g_term_info.ch_y >= g_term_info.ch_height) {
        g_term_info.ch_y_prev = 0;
        g_term_info.ch_x_prev = 0;
        g_term_info.ch_y--;
        g_term_info.ch_x = 0;
        if (g_term_info.render == TR_FRAMEBUFFER) {
            fb_scroll(1 * g_term_info.font_height);
        }
    }

    if (ch == '\n') {
        spin_unlock(&term_lock);
        return;
    }

    int x = g_term_info.ch_x * g_term_info.font_width;
    int y = g_term_info.ch_y * g_term_info.font_height;

    u8 glyph_size = _terminal_font.header.size;
    int offset = ch * glyph_size;
    psf1_font_t *font_ = g_term_info.bold ? &_terminal_font_bold : &_terminal_font;
    static const u8 masks[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
    for (int l = 0; l < glyph_size; l++) {
        u32 glyph_byte = font_->data[offset + l];
        for (int k = 0; k < PSF1_FONT_WIDTH; k++) {
            if (glyph_byte & masks[k]) {
                term_putpixel(x + k, l + y, color == 0 ? g_term_info.fgcolor : color);
            } else {
                term_putpixel(x + k, l + y, g_term_info.bgcolor);
            }
        }
    }

    g_term_info.ch_x++;

    spin_unlock(&term_lock);
}

void term_putpixel(u32 x, u32 y, u32 color) {
    if (g_term_info.render == TR_FRAMEBUFFER) {
        fb_putpixel(x, y, color);
    }
}
void term_refresh() {

    u64 flags;

    if (g_term_info.ch_y_prev == g_term_info.ch_y && 
        g_term_info.ch_x_prev == g_term_info.ch_x) {
        return;
    }

    spin_lock(&term_lock);

    int bytes_of_line = g_term_info.font_height * g_term_info.font_width * g_term_info.ch_width;
    int start = g_term_info.ch_y_prev * bytes_of_line;
    int end = g_term_info.ch_y * bytes_of_line + bytes_of_line;

    if (g_term_info.render == TR_FRAMEBUFFER) {
        fb_refresh_range(start, end);
    }

    g_term_info.ch_y_prev = g_term_info.ch_y;
    g_term_info.ch_x_prev = g_term_info.ch_x;

    spin_unlock(&term_lock);
}

void term_refresh_all() {
    spin_lock(&term_lock);

    if (g_term_info.render == TR_FRAMEBUFFER) {
        fb_refresh();
    }

    spin_unlock(&term_lock);
}

void term_set_bgcolor(u32 color) {
    spin_lock(&term_lock);
    g_term_info.bgcolor = color;
    spin_unlock(&term_lock);
}

void term_set_fgcolor(u32 color) {
    spin_lock(&term_lock);
    g_term_info.fgcolor = color;
    spin_unlock(&term_lock);
}

void term_set_bold(bool bold) {
    spin_lock(&term_lock);
    g_term_info.bold = bold;
    spin_unlock(&term_lock);
}

void term_set_lastch(char ch) {
    spin_lock(&term_lock);
    g_term_info.lastch = ch;
    spin_unlock(&term_lock);
}

void term_clear() {
    spin_lock(&term_lock);

    g_term_info.ch_x = 0;
    g_term_info.ch_y = 0;
    g_term_info.ch_y_prev = 0;
    g_term_info.ch_x_prev = 0;

    if (g_term_info.render == TR_FRAMEBUFFER) {
        fb_clear();
    }
    spin_unlock(&term_lock);

    term_refresh_all();
}

void term_back() {
    spin_lock(&term_lock);

    if (g_term_info.ch_x > 0) {
        // if (g_term_info.ch_x == g_term_info.ch_x) {
        //     g_term_info.ch_x--;
        //     g_term_info.ch_x_prev--;
        // } else {
        //     g_term_info.ch_x--;
        // }

        g_term_info.ch_x--;
        
    } else {
        // if (g_term_info.ch_y_prev == g_term_info.ch_y) {
        //     g_term_info.ch_y--;
        //     g_term_info.ch_y_prev--;
        // } else {
        //     g_term_info.ch_y--;
        // }

        // if (g_term_info.ch_x == g_term_info.ch_x) {
        //     g_term_info.ch_x = g_term_info.ch_width;
        //     g_term_info.ch_x_prev = g_term_info.ch_width;
        // } else {
        //     g_term_info.ch_x = g_term_info.ch_width;
        // }

        g_term_info.ch_y--;
        g_term_info.ch_x = g_term_info.ch_width - 1;
    }


    int x = g_term_info.ch_x * g_term_info.font_width;
    int y = g_term_info.ch_y * g_term_info.font_height;

    u8 glyph_size = _terminal_font.header.size;
    psf1_font_t *font_ = g_term_info.bold ? &_terminal_font_bold : &_terminal_font;
    static const u8 masks[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
    for (int l = 0; l < glyph_size; l++) {
        for (int k = 0; k < PSF1_FONT_WIDTH; k++) {
            term_putpixel(x + k, l + y, g_term_info.bgcolor);
        }
    }


    spin_unlock(&term_lock);
}

void term_scroll(int row) {
    spin_lock(&term_lock);

    g_term_info.ch_y_prev = 0;
    g_term_info.ch_x_prev = 0;
    g_term_info.ch_y--;
    if (g_term_info.render == TR_FRAMEBUFFER) {
        fb_scroll(row * g_term_info.font_height);
    }

    spin_unlock(&term_lock);
}

void term_newline() {
    spin_lock(&term_lock);

    g_term_info.ch_x = 0;
    g_term_info.ch_y++;

    if (g_term_info.ch_y >= g_term_info.ch_height) {
        g_term_info.ch_y_prev = 0;
        g_term_info.ch_x_prev = 0;
        g_term_info.ch_y--;
        g_term_info.ch_x = 0;
        if (g_term_info.render == TR_FRAMEBUFFER) {
            fb_scroll(1 * g_term_info.font_height);
        }
    }

    spin_unlock(&term_lock);
}

void term_set_color_push(u32 new_color) {
    spin_lock(&term_lock);

    g_term_info.old_fgcolor = g_term_info.fgcolor;
    g_term_info.fgcolor = new_color;

    spin_unlock(&term_lock);
}

void term_set_color_pop() {
    spin_lock(&term_lock);

    g_term_info.fgcolor = g_term_info.old_fgcolor;
    g_term_info.old_fgcolor = 0;

    spin_unlock(&term_lock);
}

u32 term_get_color() {
    return g_term_info.fgcolor;
}