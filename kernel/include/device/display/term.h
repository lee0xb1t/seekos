#ifndef __term_h
#define __term_h
#include <lib/ktypes.h>


typedef enum _term_render_t { TR_FRAMEBUFFER } term_render_t;

typedef struct _term_info_t {
    int ch_x, ch_y, ch_width, ch_height;
    int font_width, font_height;
    u32 fgcolor, bgcolor, prev_fgcolor;
    term_render_t render;
    int ch_x_prev, ch_y_prev;
    bool bold;
    char _unused;
} term_info_t;


#define term_color_push(c)  term_set_fgcolor_prev(c)
#define term_color_pop()    term_restore_fgcolor()
#define term_color()        term_get_fgcolor()


void term_init(term_render_t render);

void term_putch(u32 color, char ch);
void term_write(const char *s);
void term_write_n(const char *s, u32 len);
void term_back(void);
void term_newline(void);
void term_clear(void);

void term_refresh(void);
void term_flush(void);

void term_set_fgcolor(u32 color);
void term_set_bgcolor(u32 color);
void term_set_bold(bool bold);

void term_panic_write(const char *s);
void term_panic_clear(void);


#endif
