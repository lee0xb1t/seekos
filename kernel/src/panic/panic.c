#include <panic/panic.h>
#include <log/kprint.h>
#include <ia32/cpuinstr.h>
#include <lib/kcolor.h>
#include <stdarg.h>

void panic(const char *format, ...) {
    __cli();
    kpanic_clear();
    term_set_fgcolor(COLOR_RED);

    kpanic_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    kpanic_printf("panic!\n");
    kpanic_printf("---------------------------------\n");

    va_list args;
    va_start(args, format);
    {
        char buf[1024];
        kvsnprintf(buf, sizeof(buf), format, args);
        kpanic_printf("%s", buf);
    }
    va_end(args);

    kpanic_printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");
    __hang();
}
