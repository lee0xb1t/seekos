#include <device/display/term.h>
#include <device/display/psf_font.h>
#include <device/display/fb.h>
#include <lib/kcolor.h>
#include <base/spinlock.h>


extern psf1_font_t _terminal_font, _terminal_font_bold;
static term_info_t g;
DECLARE_SPINLOCK(term_lock);


static void _draw_glyph_to(void (*put)(u32, u32, u32), int px, int py, char ch, u32 fg, u32 bg) {
    u8 size = _terminal_font.header.size;
    psf1_font_t *font = g.bold ? &_terminal_font_bold : &_terminal_font;
    int off = (int)(u8)ch * size;
    static const u8 m[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

    for (int row = 0; row < size; row++) {
        u8 b = font->data[off + row];
        for (int col = 0; col < PSF1_FONT_WIDTH; col++)
            put(px + col, py + row, (b & m[col]) ? fg : bg);
    }
}

static void _draw_glyph(int px, int py, char ch, u32 fg, u32 bg) {
    if (g.render == TR_FRAMEBUFFER)
        _draw_glyph_to(fb_putpixel, px, py, ch, fg, bg);
}

static void _clear_cell(int cx, int cy) {
    if (g.render != TR_FRAMEBUFFER) return;
    int px = cx * g.font_width, py = cy * g.font_height;
    for (int y = 0; y < g.font_height; y++)
        for (int x = 0; x < g.font_width; x++)
            fb_putpixel(px + x, py + y, g.bgcolor);
}

static void _scroll_up(u32 rows) {
    if (g.render == TR_FRAMEBUFFER) fb_scroll(rows);
}

static void _advance_line(void) {
    g.ch_x_prev = g.ch_y_prev = 0;
    g.ch_x = 0; g.ch_y--;
    _scroll_up((u32)g.font_height);
}


void term_init(term_render_t render) {
    fb_info_t *info = fb_get_info();
    g.font_width  = PSF1_FONT_WIDTH;
    g.font_height = _terminal_font.header.size;
    g.ch_width    = (int)info->width  / g.font_width;
    g.ch_height   = (int)info->height / g.font_height;
    g.bgcolor     = COLOR_BLACK;
    g.fgcolor     = COLOR_WHITE;
    g.bold        = false;
    g.render      = render;
}


static void _term_putch_locked(u32 color, char ch) {
    u32 fg = color ? color : g.fgcolor;

    if (ch == '\n') { g.ch_y++; g.ch_x = 0; goto _scroll; }
    if (ch == '\t') {
        int n = 8 - (g.ch_x % 8);
        for (int i = 0; i < n; i++) _term_putch_locked(color, ' ');
        return;
    }
    if (g.ch_x >= g.ch_width) { g.ch_x = 0; g.ch_y++; }

_scroll:
    if (g.ch_y >= g.ch_height) _advance_line();
    if (ch == '\n') return;

    _draw_glyph(g.ch_x * g.font_width, g.ch_y * g.font_height, ch, fg, g.bgcolor);
    g.ch_x++;
}


void term_putch(u32 color, char ch) {
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    _term_putch_locked(color, ch);
    spin_unlock_irq(&term_lock, flags);
}


void term_write(const char *s) {
    if (!s) return;
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    while (*s)
        _term_putch_locked(0, *s++);
    spin_unlock_irq(&term_lock, flags);
}


void term_write_n(const char *s, u32 len) {
    if (!s || !len) return;
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    for (u32 i = 0; i < len; i++)
        _term_putch_locked(0, s[i]);
    spin_unlock_irq(&term_lock, flags);
}


void term_back(void) {
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    if (g.ch_x > 0) { g.ch_x--; }
    else if (g.ch_y > 0) { g.ch_y--; g.ch_x = g.ch_width - 1; }
    else { spin_unlock_irq(&term_lock, flags); return; }
    _clear_cell(g.ch_x, g.ch_y);
    spin_unlock_irq(&term_lock, flags);
}


void term_newline(void) {
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    g.ch_x = 0; g.ch_y++;
    if (g.ch_y >= g.ch_height) _advance_line();
    spin_unlock_irq(&term_lock, flags);
}


void term_clear(void) {
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    g.ch_x = g.ch_y = g.ch_x_prev = g.ch_y_prev = 0;
    if (g.render == TR_FRAMEBUFFER) fb_clear();
    spin_unlock_irq(&term_lock, flags);
    term_flush();
}


void term_refresh(void) {
    if (g.ch_y_prev == g.ch_y && g.ch_x_prev == g.ch_x) return;
    u64 flags;
    spin_lock_irq(&term_lock, flags);

    int rb = g.font_height * g.font_width * g.ch_width * FB_BYTE_SIZE_PER_PIXEL;
    int start = g.ch_y_prev * rb, end = g.ch_y * rb + rb;
    if (g.render == TR_FRAMEBUFFER) fb_flush_region((u32)start, (u32)end);

    g.ch_y_prev = g.ch_y; g.ch_x_prev = g.ch_x;
    spin_unlock_irq(&term_lock, flags);
}


void term_flush(void) {
    u64 flags;
    spin_lock_irq(&term_lock, flags);
    if (g.render == TR_FRAMEBUFFER) fb_flush();
    spin_unlock_irq(&term_lock, flags);
}


void term_set_fgcolor(u32 color) {
    u64 flags; spin_lock_irq(&term_lock, flags); g.fgcolor = color; spin_unlock_irq(&term_lock, flags);
}
void term_set_bgcolor(u32 color) {
    u64 flags; spin_lock_irq(&term_lock, flags); g.bgcolor = color; spin_unlock_irq(&term_lock, flags);
}
void term_set_bold(bool bold) {
    u64 flags; spin_lock_irq(&term_lock, flags); g.bold = bold; spin_unlock_irq(&term_lock, flags);
}
void term_set_fgcolor_prev(u32 new_color) {
    u64 flags; spin_lock_irq(&term_lock, flags); g.prev_fgcolor = g.fgcolor; g.fgcolor = new_color; spin_unlock_irq(&term_lock, flags);
}
void term_restore_fgcolor(void) {
    u64 flags; spin_lock_irq(&term_lock, flags); g.fgcolor = g.prev_fgcolor; g.prev_fgcolor = 0; spin_unlock_irq(&term_lock, flags);
}
u32 term_get_fgcolor(void) { return g.fgcolor; }


void term_panic_write(const char *s) {
    if (!s || g.render != TR_FRAMEBUFFER) return;
    g.ch_y_prev = g.ch_x_prev = 0;

    while (*s) {
        char ch = *s++;
        if (ch == '\n') { g.ch_y++; g.ch_x = 0;
            if (g.ch_y >= g.ch_height) { g.ch_y--; g.ch_x = 0; fb_scroll_nolock((u32)g.font_height); fb_flush_nolock(); }
            continue;
        }
        if (ch == '\t') { int n = 8 - (g.ch_x % 8); while (n--) term_panic_write(" "); continue; }
        if (g.ch_x >= g.ch_width) { 
            g.ch_x = 0; g.ch_y++;
            if (g.ch_y >= g.ch_height) { 
                g.ch_y--; fb_scroll_nolock((u32)g.font_height); 
            } 
        }
        _draw_glyph_to(fb_putpixel_nolock, g.ch_x * g.font_width, g.ch_y * g.font_height,
                       ch, g.fgcolor, g.bgcolor);
        g.ch_x++;
    }
    fb_flush_nolock();
}


void term_panic_clear(void) {
    g.ch_x = g.ch_y = g.ch_y_prev = g.ch_x_prev = 0;
    if (g.render == TR_FRAMEBUFFER)
        fb_fill_direct(0, 0, fb_get_width(), fb_get_height(), COLOR_BLACK);
}
