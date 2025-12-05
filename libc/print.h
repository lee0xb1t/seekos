#ifndef PRINT_H
#define PRINT_H
#include <libc/types.h>
#include <stdarg.h>


#define STDIN               0
#define STDOUT              1
#define STDERR              2


int _sprint_format_i32(char *s, int pos, i32 v);
int _sprint_format_u32(char *s, int pos, u32 v);
int _sprint_format_i64(char *s, int pos, i64 v);
int _sprint_format_u64(char *s, int pos, u64 v);
int _sprint_format_hex(char *s, int pos, u64 v, i32 wordcase);
int _sprint_format_binary(char *s, int pos, u64 v);
int _sprint_format_string(char *s, int pos, char *v);

void sprintf(char *s, int n, const char *format, ...);
void sprintv(char *s, int n, const char *format, va_list vl);
void printf(const char *format, ...);
void printv(const char *format, va_list vl);
void perrf(const char *format, ...);
void perrv(const char *format, va_list vl);


i32 _string_parse_i32(char *s);
u32 _string_parse_u32(char *s);
i64 _string_parse_i64(char *s);
u64 _string_parse_u64(char *s);
char *_string_parse_string(char *s);

int scanv(const char *format, va_list vl);
int scanf(const char *format, ...);

#endif