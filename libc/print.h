#ifndef PRINT_H
#define PRINT_H
#include <libc/types.h>
#include <stdarg.h>


#define STDIN  0
#define STDOUT 1
#define STDERR 2


int _fmt_i64(char *s, int pos, i64 v);
int _fmt_u64(char *s, int pos, u64 v, int base, int wordcase);
int _fmt_str(char *s, int pos, const char *v);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list vl);

int dprintf(int fd, const char *fmt, ...);
int vdprintf(int fd, const char *fmt, va_list vl);

int snprintf(char *s, size_t n, const char *fmt, ...);
int vsnprintf(char *s, size_t n, const char *fmt, va_list vl);

int scanf(const char *fmt, ...);
int vscanf(const char *fmt, va_list vl);

#endif
