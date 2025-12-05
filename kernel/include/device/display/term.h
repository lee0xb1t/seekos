#ifndef __term_h
#define __term_h
#include <lib/ktypes.h>
#include <stdarg.h>


typedef enum _term_render_t {
    TR_FRAMEBUFFER,
} term_render_t;

typedef struct _term_info_t{
    int ch_x;
    int ch_y;
    int ch_width;
    int ch_height;
    int font_width;
    int font_height;
    char lastch;
    u32 fgcolor;
    u32 bgcolor;
    bool bold;
    term_render_t render;
    int ch_x_prev;
    int ch_y_prev;
    u32 old_fgcolor;
} term_info_t;


#define term_color_push(new_color)    \
    do {        \
        term_set_color_push(new_color);    \
    } while(0)

#define term_color_pop()    \
    do {        \
        term_set_color_pop();    \
    } while(0)

#define term_color       term_get_color


void term_init(term_render_t render);
void term_putch(u32 color, char ch);
void term_putpixel(u32 x, u32 y, u32 color);
void term_refresh();
void term_refresh_all();
void term_clear();

void term_back();

void term_set_bgcolor(u32 color);
void term_set_fgcolor(u32 color);
void term_set_bold(bool bold);
void term_set_lastch(char ch);

void term_scroll(int row);
void term_newline();

void term_set_color_push(u32 new_color);
void term_set_color_pop();
u32 term_get_color();

#endif