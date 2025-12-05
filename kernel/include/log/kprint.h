#ifndef KPRINT_H
#define KPRINT_H
#include <lib/ktypes.h>
#include <stdarg.h>


int _ksprint_format_i32(char *s, int pos, i32 v);
int _ksprint_format_u32(char *s, int pos, u32 v);
int _ksprint_format_i64(char *s, int pos, i64 v);
int _ksprint_format_u64(char *s, int pos, u64 v);
int _ksprint_format_hex(char *s, int pos, u64 v, i32 wordcase);
int _ksprint_format_binary(char *s, int pos, u64 v);
int _ksprint_format_string(char *s, int pos, char *v);

void ksprintf(char *s, int n, const char *format, ...);
void ksprintv(char *s, int n, const char *format, va_list vl);
void kprintf(const char *format, ...);
void kprintv(const char *format, va_list vl);
void kerrf(const char *format, ...);
void kerrv(const char *format, va_list vl);

#endif