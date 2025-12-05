#ifndef STRING_H
#define STRING_H
#include <libc/types.h>


int ctol(char ch);
i32 strtol(const char *s);
i64 strtoll(const char *s);
u32 strtoul(const char *s);
u64 strtoull(const char *s);


size_t strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strlen(char *s);
char *strchr(char *str, char c);

void memset(void* dst, u8 val, size_t size);
void memcpy(void* dst, const void* src, uptr size);

#endif