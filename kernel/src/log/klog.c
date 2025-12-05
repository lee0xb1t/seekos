#include <log/klog.h>
#include <com/serial.h>
#include <lib/kcolor.h>
#include <device/display/term.h>
#include <stdarg.h>


/* Ascii Table */
char klog_at[2][16] = {
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' },
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' },
};

static volatile klog_mode_t g_log_mode = KLOG_SERIAL;
static volatile klog_level_t g_log_level = KL_TRACE;



void klog_char(u32 color, char ch) {
    if (g_log_mode == KLOG_SERIAL) {
        klog_serial_putch(ch);
    } else {
        term_putch(color, ch);
    }
}

void klog_serial_putch(char ch) {
    if (ch == '\n') {
        serial_write('\r');
    }
    serial_write((u8)ch);
}

void klog_string(u32 color, const char* str) {
    while(*str)
        klog_char(color, *str++);
}

void klog_int32(u32 color, i32 number) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 10;

    i32 temp;
    bool negative = false;
    
    if (number < 0) {
        negative = true;
        temp = (~number) + 1;
    } else {
        temp = number;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    if (negative) {
        klog_char(color, '-');
    }

    klog_string(color, buffer);
}

void klog_uint32(u32 color, u32 number) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 10;
    u32 temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    klog_string(color, buffer);
}

void klog_int64(u32 color, i64 number) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 10;
    i64 temp;
    bool negative = false;
    
    if (number < 0) {
        negative = true;
        temp = (~number) + 1;
    } else {
        temp = number;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    if (negative) {
        klog_char(color, '-');
    }

    klog_string(color, buffer);
}

void klog_uint64(u32 color, u64 number) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 10;
    u64 temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    klog_string(color, buffer);
}

void klog_hex(u32 color, u64 number, i32 wordcase) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 16;
    u64 temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[wordcase][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    klog_string(color, buffer);
}

void klog_binary(u32 color, u64 number) {
    if (number == 0) {
        klog_char(color, '0');
        return;
    }

    int base = 2;
    u64 temp = number;
    char buffer[65] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = klog_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    klog_string(color, buffer);
}

void klog_vprintf_core(u32 color, const char* format, va_list args) {
    for (int i = 0; format[i] != '\0'; i++) {
        char ch = format[i];
        switch (ch)
        {
        case '%': {
            char next = format[++i];
            switch (next) {
            case '%':
                klog_char(color, '%');
                break;
            case 'd':
                klog_int32(color, va_arg(args, i32));
                break;
            case 'q':
                klog_int64(color, va_arg(args, i64));
                break;
            case 'x':
                klog_hex(color, va_arg(args, u64), 0);
                break;
            case 'X':
                klog_hex(color, va_arg(args, u64), 1);
                break;
            case 'p':
                klog_string(color, "0x");
                klog_hex(color, va_arg(args, u64), 0);
                break;
            case 'b':
                klog_binary(color, va_arg(args, u64));
                break;
            case 's':
                klog_string(color, va_arg(args, char*));
                break;
            case 'c':
                klog_char(color, va_arg(args, char));
                break;
            case 'u': {
                switch (format[i+1])
                {
                case 'd':
                    klog_uint32(color, va_arg(args, u32));
                    ++i;
                    break;
                case 'q':
                    klog_uint64(color, va_arg(args, u64));
                    ++i;
                    break;
                
                default:
                    //klog_uint32(color, va_arg(args, u32));
                    klog_char(color, ch);
                    klog_char(color, next);
                    klog_char(color, format[i+1]);
                    break;
                }

                break;
            }   // case 'u':
            default:
                //klog_string(color, " [undefined keyword] ");
                klog_char(color, ch);
                klog_char(color, next);
                break;
            }
            
            break;
        }   // case '%':
        default:
            klog_char(color, ch);
            break;
        }
    }

    if (g_log_mode == KLOG_TERM) {
        term_refresh();
    }
}

void klog_vprintf(u32 color, const char* format, va_list args) {
    klog_vprintf_core(color, format, args);
}

void klog_set_mode(klog_mode_t mode) {
    g_log_mode = mode;
}

void klog_set_level(klog_level_t l) {
    g_log_level = l;
}

void klog_debug(const char* format, ...) {
    if (g_log_level > KL_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, format);
    
    klog_vprintf_core(term_color(), format, args);
    va_end(args);
}

void klog_info(const char* format, ...) {
    if (g_log_level > KL_INFO) {
        return;
    }
    va_list args;
    va_start(args, format);
    klog_vprintf_core(term_color(), format, args);
    va_end(args);
}

void klog_warn(const char* format, ...) {
    if (g_log_level > KL_WARN) {
        return;
    }

    va_list args;
    va_start(args, format);
    klog_vprintf_core(COLOR_YELLOW, format, args);
    va_end(args);
}

void klog_error(const char* format, ...) {
    if (g_log_level > KL_ERROR) {
        return;
    }

    va_list args;
    va_start(args, format);
    klog_vprintf_core(COLOR_RED, format, args);
    va_end(args);
}
