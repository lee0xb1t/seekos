#include <log/kprint.h>
#include <log/klog.h>
#include <com/serial.h>
#include <device/display/term.h>
#include <lib/kcolor.h>
#include <lib/kmemory.h>
#include <lib/kstring.h>
#include <mm/kmalloc.h>
#include <panic/panic.h>


#define FMT_BUF_SIZE  1024

static klog_mode_t  g_mode  = KLOG_SERIAL;
static klog_level_t g_level = KL_TRACE;

static const char hex_lower[] = "0123456789abcdef";
static const char hex_upper[] = "0123456789ABCDEF";


void klog_set_mode(klog_mode_t mode) { g_mode = mode; }
void klog_set_level(klog_level_t lv)  { g_level = lv; }

void klog_serial_putch(char ch) {
    if (ch == '\n')
        serial_write('\r');
    serial_write((u8)ch);
}


static int _fmt_u64(char *buf, int pos, u64 v, int base, const char *table) {
    if (v == 0) { buf[pos++] = '0'; return pos; }
    char tmp[32];
    int cnt = 0;
    while (v) { tmp[cnt++] = table[v % base]; v /= base; }
    while (cnt) buf[pos++] = tmp[--cnt];
    return pos;
}

static int _fmt_i64(char *buf, int pos, i64 v) {
    if (v == 0) { buf[pos++] = '0'; return pos; }
    if (v < 0) { buf[pos++] = '-'; v = -v; }
    return _fmt_u64(buf, pos, (u64)v, 10, hex_lower);
}

static int _fmt_str(char *buf, int pos, const char *s) {
    if (!s) s = "(null)";
    while (*s) buf[pos++] = *s++;
    return pos;
}


int kvsnprintf(char *buf, size_t n, const char *fmt, va_list args) {
    if (!buf || !n) return 0;
    int pos = 0;
    size_t end = n - 1;

    for (int i = 0; fmt[i] && (size_t)pos < end; i++) {
        char ch = fmt[i];

        if (ch == '\r') { pos = 0; continue; }

        if (ch != '%') { buf[pos++] = ch; continue; }

        ch = fmt[++i];
        if (!ch) break;

        if (ch == '%') { buf[pos++] = '%'; continue; }

        int lflag = 0;
        if (ch == 'l') { lflag = 1; ch = fmt[++i]; if (ch == 'l') { lflag = 2; ch = fmt[++i]; } }

        switch (ch) {
        case 'd':
            if (lflag == 2) pos = _fmt_i64(buf, pos, va_arg(args, i64));
            else if (lflag == 1) pos = _fmt_i64(buf, pos, va_arg(args, long));
            else pos = _fmt_i64(buf, pos, va_arg(args, int));
            break;
        case 'q':
            pos = _fmt_i64(buf, pos, va_arg(args, i64));
            break;
        case 'u': {
            char nxt = fmt[i + 1];
            if (nxt == 'd') { i++; pos = _fmt_u64(buf, pos, va_arg(args, u32), 10, hex_lower); }
            else if (nxt == 'q') { i++; pos = _fmt_u64(buf, pos, va_arg(args, u64), 10, hex_lower); }
            else if (lflag == 2) pos = _fmt_u64(buf, pos, va_arg(args, u64), 10, hex_lower);
            else if (lflag == 1) pos = _fmt_u64(buf, pos, va_arg(args, unsigned long), 10, hex_lower);
            else pos = _fmt_u64(buf, pos, va_arg(args, unsigned int), 10, hex_lower);
            break; }
        case 'x':
            if (lflag) pos = _fmt_u64(buf, pos, va_arg(args, u64), 16, hex_lower);
            else pos = _fmt_u64(buf, pos, va_arg(args, unsigned int), 16, hex_lower);
            break;
        case 'X':
            if (lflag) pos = _fmt_u64(buf, pos, va_arg(args, u64), 16, hex_upper);
            else pos = _fmt_u64(buf, pos, va_arg(args, unsigned int), 16, hex_upper);
            break;
        case 'p':
            buf[pos++] = '0'; buf[pos++] = 'x';
            pos = _fmt_u64(buf, pos, va_arg(args, u64), 16, hex_lower);
            break;
        case 's':
            pos = _fmt_str(buf, pos, va_arg(args, const char*));
            break;
        case 'c':
            buf[pos++] = (char)va_arg(args, int);
            break;
        case 'b':
            pos = _fmt_u64(buf, pos, va_arg(args, u64), 2, hex_lower);
            break;
        default:
            buf[pos++] = '%'; buf[pos++] = ch;
            break;
        }
    }
    buf[pos] = '\0';
    return pos;
}


int ksnprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int r = kvsnprintf(buf, n, fmt, args);
    va_end(args);
    return r;
}


int kprint_core(u32 color, const char *fmt, va_list args) {
    char buf[FMT_BUF_SIZE];
    int len = kvsnprintf(buf, sizeof(buf), fmt, args);

    if (g_mode == KLOG_SERIAL) {
        for (int i = 0; i < len; i++)
            klog_serial_putch(buf[i]);
    } else {
        term_color_push(color);
        term_write(buf);
        term_color_pop();
        term_flush();
    }
    return len;
}


int kprintf_color(u32 color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int r = kprint_core(color, fmt, args);
    va_end(args);
    return r;
}


u32 klog_level_color(klog_level_t lv) {
    switch (lv) {
    case KL_TRACE: return COLOR_GARY;
    case KL_DEBUG: return COLOR_WHITE;
    case KL_INFO:  return COLOR_GREEN;
    case KL_WARN:  return COLOR_YELLOW;
    case KL_ERROR: return COLOR_RED;
    default:       return COLOR_WHITE;
    }
}

void klog_write(klog_level_t lv, const char *fmt, ...) {
    if (lv < g_level) return;

    va_list args;
    va_start(args, fmt);
    kprint_core(klog_level_color(lv), fmt, args);
    va_end(args);
}
