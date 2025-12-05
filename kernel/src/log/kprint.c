#include <log/kprint.h>
#include <lib/ktypes.h>
#include <lib/kmemory.h>
#include <lib/kstring.h>
#include <mm/kmalloc.h>
#include <fs/vfs.h>
#include <panic/panic.h>
#include <stdarg.h>


#define KPRINT_MAX_BUFFER_SIZE      4096

/* Ascii Table */
char kprint_at[2][16] = {
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' },
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' },
};

int _ksprint_format_i32(char *s, int pos, i32 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 10;

    i32 temp;
    bool negative = false;
    
    if (v < 0) {
        negative = true;
        temp = (~v) + 1;
    } else {
        temp = v;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[0][number_bit];
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
        s[pos++] = '-';
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_u32(char *s, int pos, u32 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 10;

    u32 temp;
    bool negative = false;
    
    if (v < 0) {
        negative = true;
        temp = (~v) + 1;
    } else {
        temp = v;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[0][number_bit];
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
        s[pos++] = '-';
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_i64(char *s, int pos, i64 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 10;
    i64 temp;
    bool negative = false;
    
    if (v < 0) {
        negative = true;
        temp = (~v) + 1;
    } else {
        temp = v;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[0][number_bit];
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
        s[pos++] = '-';
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_u64(char *s, int pos, u64 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 10;
    u64 temp;
    bool negative = false;
    
    if (v < 0) {
        negative = true;
        temp = (~v) + 1;
    } else {
        temp = v;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[0][number_bit];
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
        s[pos++] = '-';
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_hex(char *s, int pos, u64 v, i32 wordcase) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 16;
    u64 temp = v;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[wordcase][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_binary(char *s, int pos, u64 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }

    int base = 2;
    u64 temp = v;
    char buffer[65] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = kprint_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    return _ksprint_format_string(s, pos, buffer);
}

int _ksprint_format_string(char *s, int pos, char *v) {
    while (*v) {
        s[pos++] = *v++;
    }
    return pos;
}

void ksprintf(char *s, int n, const char *format, ...) {
    va_list vl;
    va_start(vl, format);
    ksprintv(s, n, format, vl);
    va_end(vl);
}

void ksprintv(char *s, int n, const char *format, va_list vl) {
    int pos = 0;
    for (int i = 0; format[i] != '\0'; i++) {
        if (pos >= n) {
            panic("[KPRINT] buffer is small");
        }
        char ch = format[i];
        switch (ch)
        {
        case '%': {
            char next = format[++i];
            switch (next)
            {
            case '%':
                //klog_char(color, '%');
                s[pos++] = '%';
                break;
            case 'd':
                //klog_int32(color, va_arg(vl, i32));
                pos = _ksprint_format_i32(s, pos, va_arg(vl, i32));
                break;
            case 'q':
                //klog_int64(color, va_arg(vl, i64));
                pos = _ksprint_format_i64(s, pos, va_arg(vl, i64));
                break;
            case 'x':
                //klog_hex(color, va_arg(vl, u64), 0);
                pos = _ksprint_format_hex(s, pos, va_arg(vl, u64), 0);
                break;
            case 'X':
                //klog_hex(color, va_arg(vl, u64), 1);
                pos = _ksprint_format_hex(s, pos, va_arg(vl, u64), 1);
                break;
            case 'p':
                //klog_string(color, "0x");
                pos = _ksprint_format_string(s, pos, "0x");
                // klog_hex(color, va_arg(vl, u64), 0);
                pos = _ksprint_format_hex(s, pos, va_arg(vl, u64), 0);
                break;
            case 'b':
                // klog_binary(color, va_arg(vl, u64));
                pos = _ksprint_format_binary(s, pos, va_arg(vl, u64));
                break;
            case 's':
                //klog_string(color, va_arg(vl, char*));
                pos = _ksprint_format_string(s, pos, va_arg(vl, char*));
                break;
            case 'c':
                //klog_char(color, va_arg(vl, char));
                s[pos++] = va_arg(vl, char);
                break;
            case 'u': {
                switch (format[i+1])
                {
                case 'd':
                    pos = _ksprint_format_u32(s, pos, va_arg(vl, u32));
                    ++i;
                    break;
                case 'q':
                    pos = _ksprint_format_u64(s, pos, va_arg(vl, u64));
                    ++i;
                    break;
                default:
                    s[pos++] = ch;
                    s[pos++] = next;
                    break;
                }
                break;
            }
            default:
                s[pos++] = ch;
                s[pos++] = next;
                break;
            }
            
            break;
        }   // case '%':
        default:
            s[pos++] = ch;
            break;
        }
    }
}

void kprintf(const char *format, ...) {
    va_list vl;
    va_start(vl, format);
    kprintv(format, vl);
    va_end(vl);
}

void kprintv(const char *format, va_list vl) {
    char *s = (char *)kzalloc(KPRINT_MAX_BUFFER_SIZE);
    ksprintv(s, KPRINT_MAX_BUFFER_SIZE, format, vl);
    vfs_handle_t fh = vfs_open_console("/dev/ttyfs", VFS_STDOUT);
    vfs_write(fh, strlen(s), s);
    vfs_close(fh);
    kfree(s);
}

void kerrf(const char *format, ...) {
    va_list vl;
    va_start(vl, format);
    kerrv(format, vl);
    va_end(vl);
}

void kerrv(const char *format, va_list vl) {
    char *s = (char *)kzalloc(KPRINT_MAX_BUFFER_SIZE);
    ksprintv(s, KPRINT_MAX_BUFFER_SIZE, format, vl);
    vfs_handle_t fh = vfs_open_console("/dev/ttyfs", VFS_STDERR);
    vfs_write(fh, strlen(s), s);
    vfs_close(fh);
    kfree(s);
}
