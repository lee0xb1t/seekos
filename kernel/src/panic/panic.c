#include <panic/panic.h>
#include <log/klog.h>
#include <ia32/cpuinstr.h>
#include <lib/kcolor.h>
#include <stdarg.h>

void panic_start() {
    kloge(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    kloge("panic!\n");
    kloge("---------------------------------\n");
}

void panic_end() {
    kloge("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");
    __hang();
}

void panic(const char* format, ...) {
    __cli();
    panic_start();
    va_list args;
    va_start(args, format);
    klog_vprintf(COLOR_RED, format, args);
    va_end(args);
    panic_end();
}
