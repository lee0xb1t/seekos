#include <log/kprint.h>
#include <lib/kcolor.h>
#include <device/display/term.h>
#include <stdarg.h>

#undef klog_debug
#undef klog_info
#undef klog_warn
#undef klog_error
#undef kprintf
#undef kerrf
#undef klogd
#undef klogi
#undef klogw
#undef kloge

void klog_debug(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(COLOR_WHITE, fmt, args);
    va_end(args);
}
void klog_info(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(COLOR_GREEN, fmt, args);
    va_end(args);
}
void klog_warn(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(COLOR_YELLOW, fmt, args);
    va_end(args);
}
void klog_error(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(COLOR_RED, fmt, args);
    va_end(args);
}

int kprintf(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(term_color(), fmt, args);
    va_end(args);
    return 0;
}
int kerrf(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    kprint_core(COLOR_RED, fmt, args);
    va_end(args);
    return 0;
}

void klog_vprintf(u32 color, const char *fmt, va_list args) {
    kprint_core(color, fmt, args);
}


void kpanic_printf(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    term_panic_write(buf);
}

void kpanic_clear(void) {
    term_panic_clear();
}
