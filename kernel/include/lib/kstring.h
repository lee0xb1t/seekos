#ifndef __KSTRING_H
#define __KSTRING_H
#include <lib/ktypes.h>

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t len);
int strcmp(const char *str1, const char *str2);

#endif