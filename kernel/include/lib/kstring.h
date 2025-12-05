#ifndef __KSTRING_H
#define __KSTRING_H
#include <lib/ktypes.h>

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t len);
bool strcmp(char *dest, const char *src);

#endif