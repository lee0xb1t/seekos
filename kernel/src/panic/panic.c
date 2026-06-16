#include <panic/panic.h>
#include <log/kprint.h>
#include <ia32/cpuinstr.h>
#include <lib/kcolor.h>
#include <device/display/term.h>
#include <com/serial.h>
#include <stdarg.h>

void panic(const char *format, ...) {
    va_list args;
    char buf[1024];

    va_start(args, format);
    kvsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    __cli();
    kpanic_clear();
    term_set_fgcolor(COLOR_RED);

    kpanic_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    kpanic_printf("panic!\n");
    kpanic_printf("---------------------------------\n");
    kpanic_printf("%s", buf);
    kpanic_printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");

    serial_write_str(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    serial_write_str("panic!\n");
    serial_write_str("---------------------------------\n");
    serial_write_str(buf);
    serial_write_str("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

    __hang();
}
