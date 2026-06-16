#include <libc/print.h>
#include <libc/errno.h>
#include <libc/sysfunc.h>
#include <libc/string.h>
#include <libc/math.h>
#include <libc/types.h>


#define PRINT_BUF_SIZE  4096

static const char hex_lower[] = "0123456789abcdef";
static const char hex_upper[] = "0123456789ABCDEF";


int _fmt_u64(char *s, int pos, u64 v, int base, int wordcase) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }
    const char *table = wordcase ? hex_upper : hex_lower;
    char tmp[32];
    int cnt = 0;
    while (v) {
        tmp[cnt++] = table[v % base];
        v /= base;
    }
    while (cnt)
        s[pos++] = tmp[--cnt];
    return pos;
}

int _fmt_i64(char *s, int pos, i64 v) {
    if (v == 0) {
        s[pos++] = '0';
        return pos;
    }
    if (v < 0) {
        s[pos++] = '-';
        v = -v;
    }
    return _fmt_u64(s, pos, (u64)v, 10, 0);
}

int _fmt_str(char *s, int pos, const char *v) {
    if (!v) v = "(null)";
    while (*v)
        s[pos++] = *v++;
    return pos;
}


int vsnprintf(char *s, size_t n, const char *fmt, va_list vl) {
    if (!s || !n) return 0;
    int pos = 0;
    size_t end = n - 1;

    for (int i = 0; fmt[i] && (size_t)pos < end; i++) {
        char ch = fmt[i];
        if (ch != '%') {
            s[pos++] = ch;
            continue;
        }

        ch = fmt[++i];
        if (!ch) break;
        if (ch == '%') {
            s[pos++] = '%';
            continue;
        }

        int lflag = 0;
        if (ch == 'l') {
            lflag = 1; ch = fmt[++i];
            if (ch == 'l') { lflag = 2; ch = fmt[++i]; }
        }

        switch (ch) {
        case 'd':
            if (lflag) pos = _fmt_i64(s, pos, va_arg(vl, i64));
            else       pos = _fmt_i64(s, pos, va_arg(vl, i32));
            break;
        case 'u':
            if (lflag) pos = _fmt_u64(s, pos, va_arg(vl, u64), 10, 0);
            else       pos = _fmt_u64(s, pos, va_arg(vl, u32), 10, 0);
            break;
        case 'x':
            if (lflag) pos = _fmt_u64(s, pos, va_arg(vl, u64), 16, 0);
            else       pos = _fmt_u64(s, pos, va_arg(vl, u32), 16, 0);
            break;
        case 'X':
            if (lflag) pos = _fmt_u64(s, pos, va_arg(vl, u64), 16, 1);
            else       pos = _fmt_u64(s, pos, va_arg(vl, u32), 16, 1);
            break;
        case 'p':
            s[pos++] = '0'; s[pos++] = 'x';
            pos = _fmt_u64(s, pos, va_arg(vl, u64), 16, 0);
            break;
        case 's':
            pos = _fmt_str(s, pos, va_arg(vl, const char*));
            break;
        case 'c':
            s[pos++] = (char)va_arg(vl, int);
            break;
        case 'b':
            pos = _fmt_u64(s, pos, va_arg(vl, u64), 2, 0);
            break;
        default:
            s[pos++] = '%'; s[pos++] = ch;
            break;
        }
    }
    s[pos] = '\0';
    return pos;
}

int snprintf(char *s, size_t n, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int r = vsnprintf(s, n, fmt, vl);
    va_end(vl);
    return r;
}


int vdprintf(int fd, const char *fmt, va_list vl) {
    char *buf = (char *)sys_vmalloc(null, PRINT_BUF_SIZE);
    if (!buf) return -1;
    memset(buf, 0, PRINT_BUF_SIZE);
    int len = vsnprintf(buf, PRINT_BUF_SIZE, fmt, vl);
    sys_write(fd, len, buf);
    sys_vfree(buf);
    return len;
}

int dprintf(int fd, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int r = vdprintf(fd, fmt, vl);
    va_end(vl);
    return r;
}


int vprintf(const char *fmt, va_list vl) {
    return vdprintf(STDOUT, fmt, vl);
}

int printf(const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int r = vdprintf(STDOUT, fmt, vl);
    va_end(vl);
    return r;
}


int vscanf(const char *fmt, va_list vl) {
    char rbuf[4096];
    for (int i = 0; fmt[i] != '\0'; i++) {
        char ch = fmt[i];
        if (ch != '%') continue;

        ch = fmt[++i];
        if (!ch) break;

        int lflag = 0;
        if (ch == 'l') {
            lflag = 1; ch = fmt[++i];
            if (ch == 'l') { lflag = 2; ch = fmt[++i]; }
        }

        switch (ch) {
        case 'd': {
            memset(rbuf, 0, 4096);
            sys_read(STDIN, 4096, rbuf);
            if (lflag) {
                i64 v = strtoll(rbuf);
                memcpy(va_arg(vl, i64*), &v, sizeof(i64));
            } else {
                i32 v = strtol(rbuf);
                memcpy(va_arg(vl, i32*), &v, sizeof(i32));
            }
            break;
        }
        case 'u': {
            memset(rbuf, 0, 4096);
            sys_read(STDIN, 4096, rbuf);
            if (lflag) {
                u64 v = strtoull(rbuf);
                memcpy(va_arg(vl, u64*), &v, sizeof(u64));
            } else {
                u32 v = strtoul(rbuf);
                memcpy(va_arg(vl, u32*), &v, sizeof(u32));
            }
            break;
        }
        case 's':
            memset(rbuf, 0, 4096);
            sys_read(STDIN, 4095, rbuf);
            rbuf[4095] = '\0';
            memcpy(va_arg(vl, char *), rbuf, min(strlen(rbuf), (size_t)255));
            break;
        default:
            break;
        }
    }
    return 0;
}

int scanf(const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int r = vscanf(fmt, vl);
    va_end(vl);
    return r;
}
